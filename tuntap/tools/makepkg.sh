#!/bin/sh
#
# calls PackageMaker to build the package
#
pwd=`pwd`
pkgmkr=/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker

# make a copy of the root directory, and change ownership as required
root=$2
roottmp="${root}_tmp"

cp -R $root $roottmp
sudo chown -R root:wheel $roottmp || echo -e '\n\nInvalid password!\n\n'

# create the package
rm -rf $pwd/$1
$pkgmkr -build -p $pwd/$1 -f $pwd/$roottmp/ -r $3 -i $pwd/$4/Info.plist -d $pwd/$4/Description.plist

sudo rm -rf $roottmp

exit 0

