#!/bin/bash

set -e
unset SOURCE_DATE_EPOCH

# Manually create the AppImage (reproducibly) since linuxdeployqt is not able to create cross-compiled AppImages

APPDIR="$PWD/BestWallet.AppDir"

mkdir -p "$APPDIR"
mkdir -p "$APPDIR/usr/share/applications/"
mkdir -p "$APPDIR/usr/bin"

cp "src/assets/BestWallet.desktop" "$APPDIR/usr/share/applications/BestWallet.desktop"
cp "src/assets/BestWallet.desktop" "$APPDIR/BestWallet.desktop"
cp "src/assets/images/appicons/64x64.png" "$APPDIR/BestWallet.png"
cp "build/bin/BestWallet" "$APPDIR/usr/bin/BestWallet"
chmod +x "$APPDIR/usr/bin/BestWallet"

cp "contrib/AppImage/AppRun" "$APPDIR/"
chmod +x "$APPDIR/AppRun"

find BestWallet.AppDir/ -exec touch -h -a -m -t 202101010100.00 {} \;

mksquashfs BestWallet.AppDir BestWallet.squashfs -comp zstd -info -root-owned -no-xattrs -noappend -fstime 0
# mksquashfs writes a timestamp to the header
printf '\x00\x00\x00\x00' | dd conv=notrunc of=BestWallet.squashfs bs=1 seek=$((0x8))

rm -f BestWallet.AppImage

cat /bestwallet/contrib/depends/${HOST}/runtime >> BestWallet.AppImage
cat BestWallet.squashfs >> BestWallet.AppImage
chmod a+x BestWallet.AppImage
