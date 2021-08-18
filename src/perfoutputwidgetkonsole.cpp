/*
  perfoutputwidgetkonsole.cpp

  This file is part of Hotspot, the Qt GUI for performance analysis.

  Copyright (C) 2021 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Lieven Hey <lieven.hey@kdab.com>

  Licensees holding valid commercial KDAB Hotspot licenses may use this file in
  accordance with Hotspot Commercial License Agreement provided with the Software.

  Contact info@kdab.com if any conditions of this licensing are not clear to you.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "perfoutputwidgetkonsole.h"

#include <QEvent>
#include <QKeyEvent>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QVBoxLayout>

#include <KParts/ReadOnlyPart>
#include <KParts/kde_terminal_interface.h>
#include <KService>

PerfOutputWidgetKonsole::PerfOutputWidgetKonsole(QWidget* parent)
    : PerfOutputWidget(parent)
{
    auto layout = new QVBoxLayout(this);
    setLayout(layout);
}

PerfOutputWidgetKonsole::~PerfOutputWidgetKonsole() = default;

PerfOutputWidgetKonsole* PerfOutputWidgetKonsole::create(QWidget* parent)
{
    auto part = createPart();
    if (!part) {
        return nullptr;
    }

    delete part;

    if (findTail().isEmpty()) {
        return nullptr;
    }

    return new PerfOutputWidgetKonsole(parent);
}

bool PerfOutputWidgetKonsole::eventFilter(QObject* watched, QEvent* event)
{
    Q_UNUSED(watched);
    if (event->type() == QEvent::KeyPress) {
        auto keyEvent = static_cast<QKeyEvent*>(event);

        // ignore ctrl + c
        if (keyEvent->key() == Qt::Key_C && keyEvent->modifiers() == Qt::ControlModifier) {
            return true;
        }

        if (m_inputEnabled) {
            m_inputBuffer.append(keyEvent->text().toUtf8());

            if (keyEvent->key() == Qt::Key_Enter) {
                emit sendInput(m_inputBuffer);
                m_inputBuffer.clear();
                return false;
            }
        } else {
            // eat all key events if input is disabled
            return true;
        }
    } else if (event->type() == QEvent::MouseButtonPress) {
        auto mouseEvent = static_cast<QMouseEvent*>(event);

        // prevent the user from opening the context menu
        if (mouseEvent->button() == Qt::MouseButton::RightButton) {
            return true;
        }
    }
    return false;
}

void PerfOutputWidgetKonsole::addOutput(const QString& output)
{
    m_konsoleFile->write(output.toUtf8());
    m_konsoleFile->flush();
}

void PerfOutputWidgetKonsole::clear()
{
    if (m_konsolePart) {
        m_konsolePart->deleteLater();
    }

    m_konsolePart = createPart();
    Q_ASSERT(m_konsolePart);

    if (m_konsoleFile) {
        m_konsoleFile->deleteLater();
    }
    m_konsoleFile = new QTemporaryFile(this);
    m_konsoleFile->open();

    const auto tail = findTail();
    Q_ASSERT(!tail.isEmpty());

    const auto terminalInterface = qobject_cast<TerminalInterface*>(m_konsolePart);
    if (!terminalInterface) {
        qWarning("konsole kpart doesn't implement terminal interface");
        delete m_konsolePart;
        m_konsolePart = nullptr;
        return;
    }

    terminalInterface->startProgram(tail, {tail, QStringLiteral("-f"), m_konsoleFile->fileName()});

    m_konsolePart->widget()->focusWidget()->installEventFilter(this);

    layout()->addWidget(m_konsolePart->widget());
}

void PerfOutputWidgetKonsole::enableInput(bool enable)
{
    if (enable != m_inputEnabled) {
        m_inputEnabled = enable;
        m_inputBuffer.clear();
    }
}

void PerfOutputWidgetKonsole::setInputVisible(bool visible)
{
    Q_UNUSED(visible);
}

QString PerfOutputWidgetKonsole::findTail()
{
    const auto tail = QStandardPaths::findExecutable(QStringLiteral("tail"));

    return tail;
}

KParts::ReadOnlyPart* PerfOutputWidgetKonsole::createPart()
{
    KService::Ptr service = KService::serviceByDesktopName(QStringLiteral("konsolepart"));

    if (!service) {
        return nullptr;
    }
    return service->createInstance<KParts::ReadOnlyPart>();
}
