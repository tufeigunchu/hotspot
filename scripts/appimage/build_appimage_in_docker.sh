#!/bin/sh
#
# SPDX-FileCopyrightText: Milian Wolff <milian.wolff@kdab.com>
# SPDX-FileCopyrightText: 2022 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
#
# SPDX-License-Identifier: GPL-2.0-or-later
#

set -e

cd "$(dirname $0)"

mkdir -p ../output/build-appimage

docker run -it --privileged --device /dev/fuse --cap-add SYS_ADMIN \
    -v $PWD/../output:/output \
    -v $PWD/../../:/hotspot \
    ghcr.io/kdab/kdesrc-build:latest \
    /hotspot/scripts/appimage/build_appimage.sh /hotspot /output/build-appimage
