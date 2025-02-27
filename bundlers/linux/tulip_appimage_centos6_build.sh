#!/bin/bash

cd

# clean packages cache
yum -y clean all
yum -y clean expire-cache
yum -y update

# install base build system
yum -y install epel-release
yum -y install xz cmake3 tar gzip make wget ccache

# if needed install GCC 7 as OGDF requires a quite advanced C++11 compiler
which gcc > /dev/null 2>&1
if [ ! $? -eq 0 ]; then
  yum -y install centos-release-scl
  yum -y install devtoolset-7
  COMPILER_DEFINES="-DCMAKE_C_COMPILER=/opt/rh/devtoolset-7/root/usr/bin/gcc -DCMAKE_CXX_COMPILER=/opt/rh/devtoolset-7/root/usr/bin/g++"
fi

# install tulip deps
yum -y install zlib-devel cppunit-devel
yum -y install freetype-devel glew-devel

# needed for generating the AppImage
yum -y install fuse fuse-libs

# set USR_LIB (needed to set CMAKE_PREFIX_PATH)
if [ "$(uname -p)" == "x86_64" ]; then
  USR_LIB=/usr/lib64
else
  USR_LIB=/usr/lib
fi

if [ "$3" == "" ]; then
  # install qt5
  yum install -y qt5-qtbase-devel qt5-qtwebkit-devel
  QT_PATH=$USR_LIB/qt5
else
  # we can use our own build of qt5
  QT_PATH=$3
fi

# install Python 3.6 needed packages
if [ "$PYTHON_EXECUTABLE" == "" ]; then
  yum -y install rh-python36-python rh-python36-python-devel rh-python36-python-sphinx
  PYTHON_EXECUTABLE=$(scl enable rh-python36 'which python3.6')
fi

# build and install tulip
if [ -d /tulip/build ]; then
  rm -rf /tulip/build/*
else
  mkdir /tulip/build
fi

cd /tulip/build
if [ "$1" == "NO_CCACHE" ]; then
  CCACHE=OFF
else
  CCACHE=ON
fi
if [ "$2" == "RUN_TESTS" ]; then
  RUN_TESTS=ON
else
  RUN_TESTS=OFF
fi

cmake3 $COMPILER_DEFINES -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$PWD/install -DCMAKE_PREFIX_PATH=$QT_PATH -DPYTHON_EXECUTABLE=$PYTHON_EXECUTABLE -DTULIP_USE_CCACHE=$CCACHE -DTULIP_BUILD_FOR_APPIMAGE=ON -DTULIP_BUILD_TESTS=$RUN_TESTS ..
make -j4 install

# run unit tests
if [ "$2" == "RUN_TESTS" ]; then
  make runTests
fi

PERSPECTIVE=Tulip
if [ "$4" == "" ]; then
  PERSPECTIVE=Tulip
else
  PERSPECTIVE=$4
fi

# build a bundle dir suitable for AppImageKit
sh bundlers/linux/make_appimage_bundle.sh --appdir $PWD --perspective $PERSPECTIVE

# get latest released appimagetool
wget "https://github.com/probonopd/AppImageKit/releases/download/12/appimagetool-$(uname -p).AppImage"
chmod a+x appimagetool-$(uname -p).AppImage

# finally build the portable app
./appimagetool-$(uname -p).AppImage $PERSPECTIVE.AppDir $PERSPECTIVE-$(sh tulip-config --version)-$(uname -p).AppImage
