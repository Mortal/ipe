#! /bin/bash
# Contributed by 'leahcimb', August 2011.

IPE_VERSION=7.1.5

## Create directories for .app bundle
APPDIR="$( cd "$( dirname "$0" )" && cd .. && pwd )"/Ipe.app
APPFRAMEWORKS=$APPDIR/Contents/Frameworks
APPMACOS=$APPDIR/Contents/MacOS
APPPLUGINS=$APPDIR/Contents/PlugIns
APPRESOURCES=$APPDIR/Contents/Resources
APPLOCAL=$APPDIR/Contents/Resources/local
mkdir -p $APPLOCAL/bin
mkdir -p $APPLOCAL/lib
mkdir -p $APPMACOS
mkdir -p $APPFRAMEWORKS
mkdir -p $APPPLUGINS

## Create config files for .app bundle.
# Info.plist
cat <<EOF > $APPDIR/Contents/Info.plist
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleDocumentTypes</key>
	<array>
		<dict>
			<key>CFBundleTypeName</key>
			<string>Ipe figure</string>
			<key>CFBundleTypeIconFile</key>
			<string>ipe.icns</string>
			<key>LSHandlerRank</key>
			<string>Owner</string>
			<key>CFBundleTypeRole</key>
			<string>Editor</string>
			<key>CFBundleTypeExtensions</key>
			<array>
				<string>ipe</string>
				<string>IPE</string>
			</array>
		</dict>
		<dict>
			<key>CFBundleTypeName</key>
			<string>Postscript document</string>
			<key>CFBundleTypeIconFile</key>
			<string>ipe.icns</string>
			<key>LSHandlerRank</key>
			<string>Alternate</string>
			<key>CFBundleTypeRole</key>
			<string>Editor</string>
			<key>CFBundleTypeExtensions</key>
			<array>
				<string>eps</string>
				<string>EPS</string>
			</array>
		</dict>
		<dict>
			<key>CFBundleTypeName</key>
			<string>PDF document</string>
			<key>CFBundleTypeIconFile</key>
			<string>ipe.icns</string>
			<key>LSHandlerRank</key>
			<string>Alternate</string>
			<key>CFBundleTypeRole</key>
			<string>Editor</string>
			<key>CFBundleTypeExtensions</key>
			<array>
				<string>pdf</string>
				<string>PDF</string>
			</array>
			<key>CFBundleTypeMIMETypes</key>
			<array>
				<string>application/pdf</string>
			</array>
		</dict>
	</array>
	<key>CFBundleDevelopmentRegion</key>
	<string>English</string>
	<key>CFBundleExecutable</key>
	<string>ipe</string>
	<key>CFBundleIconFile</key>
	<string>ipe.icns</string>
	<key>CFBundleIdentifier</key>
	<string>net.sourceforge.ipe7</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundleName</key>
	<string>Ipe</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>CFBundleShortVersionString</key>
	<string>$IPE_VERSION</string>
	<key>CFBundleSignature</key>
	<string>Ipe7</string>
	<key>CFBundleVersion</key>
	<string>$IPE_VERSION</string>
	<key>NSHumanReadableCopyright</key>
	<string>Copyright (C) 1993-2013  Otfried Cheong</string>
</dict>
</plist>
EOF
# ipe startup script
echo '#! /bin/bash

# IPELATEXDIR
# IPEFONTMAP
# IPEDEBUG
# IPESTYLES
# IPELETPATH
# IPEICONDIR
# IPEDOCDIR
# IPELUAPATH
# EDITOR

DIR="$( cd "$( dirname "$0" )" && cd .. && pwd )"/Resources/local/bin
PATH=$DIR:/usr/texbin:/usr/local/texbin:/usr/local/bin:$PATH

(cd $DIR; ./ipe)
' > $APPDIR/Contents/MacOS/ipe
chmod +x $APPDIR/Contents/MacOS/ipe
# qt.conf
echo '[Paths]
Plugins = PlugIns
' > $APPDIR/Contents/Resources/qt.conf
ln -s ../../../Resources/qt.conf $APPLOCAL/bin/qt.conf

## Copy Qt frameworks into .app bundle and set relative links
cp -R /Library/Frameworks/QtCore.framework $APPFRAMEWORKS/
cp -R /Library/Frameworks/QtGui.framework $APPFRAMEWORKS/
cp -R /Library/Frameworks/QtNetwork.framework $APPFRAMEWORKS/

install_name_tool -id @executable_path/../../../Frameworks/QtCore.framework/Versions/4/QtCore $APPFRAMEWORKS/QtCore.framework/Versions/4/QtCore
install_name_tool -id @executable_path/../../../Frameworks/QtGui.framework/Versions/4/QtGui $APPFRAMEWORKS/QtGui.framework/Versions/4/QtGui
install_name_tool -id @executable_path/../../../Frameworks/QtNetwork.framework/Versions/4/QtNetwork $APPFRAMEWORKS/QtNetwork.framework/Versions/4/QtNetwork

FIX_QTCORE='install_name_tool -change QtCore.framework/Versions/4/QtCore @executable_path/../../../Frameworks/QtCore.framework/Versions/4/QtCore'
FIX_QTGUI='install_name_tool -change QtGui.framework/Versions/4/QtGui @executable_path/../../../Frameworks/QtGui.framework/Versions/4/QtGui'
FIX_QTNETWORK='install_name_tool -change QtNetwork.framework/Versions/4/QtNetwork @executable_path/../../../Frameworks/QtNetwork.framework/Versions/4/QtNetwork'

$FIX_QTCORE $APPFRAMEWORKS/QtGui.framework/Versions/4/QtGui
$FIX_QTCORE $APPFRAMEWORKS/QtNetwork.framework/Versions/4/QtNetwork

cp -R /Developer/Applications/Qt/plugins/accessible $APPPLUGINS/
cp -R /Developer/Applications/Qt/plugins/codecs $APPPLUGINS/
cp -R /Developer/Applications/Qt/plugins/graphicssystems $APPPLUGINS/
cp -R /Developer/Applications/Qt/plugins/imageformats $APPPLUGINS/
rm -rf $APPPLUGINS/accessible/libqtaccessiblecompatwidgets.dylib
rm -rf $APPPLUGINS/graphicssystems/libqglgraphicssystem.dylib
rm -rf $APPPLUGINS/imageformats/libqsvg.dylib

## Copy lua library
cp /usr/local/lib/liblua52.dylib $APPLOCAL/lib/
install_name_tool -id @executable_path/../lib/liblua52.dylib $APPLOCAL/lib/liblua52.dylib

## Build ipe
export INSTALL_ROOT=$APPDIR/Contents/Resources/1/2/
export LUA_LIBS="-L$APPDIR/Contents/Resources/local/lib -llua52 -lm"
export IPEPREFIX=../../local
export IPELIBDIRINFO=@executable_path/../lib

make && make install

rm -rf $APPRESOURCES/1

## Fix dynamic library links
find $APPLOCAL -d -type f -exec $FIX_QTCORE {} \; &> /dev/null
find $APPLOCAL -d -type f -exec $FIX_QTGUI {} \; &> /dev/null
find $APPLOCAL -d -type f -exec $FIX_QTNETWORK {} \; &> /dev/null
find $APPPLUGINS -d -type f -exec $FIX_QTCORE {} \; &> /dev/null
find $APPPLUGINS -d -type f -exec $FIX_QTGUI {} \; &> /dev/null
find $APPPLUGINS -d -type f -exec $FIX_QTNETWORK {} \; &> /dev/null

## Create icon
sips -s format icns $APPLOCAL/share/ipe/$IPE_VERSION/icons/ipe.png --out $APPRESOURCES/ipe.icns
