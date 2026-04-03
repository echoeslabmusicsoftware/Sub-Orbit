#!/bin/bash
set -euo pipefail

VERSION="${1:?Usage: build-deb.sh <version> <vst3-path>}"
VST3_SOURCE="${2:?Usage: build-deb.sh <version> <vst3-path>}"

PKG_DIR="suborbit_${VERSION}_amd64"

mkdir -p "${PKG_DIR}/DEBIAN"
mkdir -p "${PKG_DIR}/usr/lib/vst3"

cp -r "${VST3_SOURCE}" "${PKG_DIR}/usr/lib/vst3/"

INSTALLED_SIZE=$(du -sk "${PKG_DIR}/usr" | cut -f1)

cat > "${PKG_DIR}/DEBIAN/control" <<EOF
Package: suborbit
Version: ${VERSION}
Architecture: amd64
Maintainer: Echoes Lab <hello@echoeslabmusic.com>
Homepage: https://github.com/echoeslabmusicsoftware/Sub-Orbit
Description: SUB ORBIT - Mono-safe low-end stereoizer
 Widens bass frequencies using quadrature allpass stereoization
 while preserving full mono compatibility. VST3 audio plugin.
Section: sound
Priority: optional
Installed-Size: ${INSTALLED_SIZE}
EOF

dpkg-deb --build --root-owner-group "${PKG_DIR}"
echo "Built ${PKG_DIR}.deb"
