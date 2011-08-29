#! /bin/bash
# Contributed by 'leahcimb', August 2011.

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
echo '{
	CFBundleIconFile = ipe;
	CFBundleName = ipe;
	CFBundleDisplayName = Ipe;
	CFBundleIdentifier = "local.ipe";
	CFBundleVersion = "7.1.0";
	CFBundlePackageType = APPL;
	CFBundleSignature = ipee;
	CFBundleExecutable = ipe;
}
' > $APPDIR/Contents/Info.plist
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
cp /usr/local/lib/liblua5.1.dylib $APPLOCAL/lib/
install_name_tool -id @executable_path/../lib/liblua5.1.dylib $APPLOCAL/lib/liblua5.1.dylib

## Build ipe
export INSTALL_ROOT=$APPDIR/Contents/Resources/1/2/
export LUA_LIBS="-L$APPDIR/Contents/Resources/local/lib -llua5.1 -lm"
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
sips -s format icns $APPLOCAL/share/ipe/7.1.0/icons/ipe.png --out $APPRESOURCES/ipe.icns
