#!/usr/bin/env bash

set -o errexit  # Exit the script with error if any of the commands fail

# Supported/used environment variables:
#   CMAKE                      Path to cmake executable.
#   BSON_ONLY                  Whether to build only the BSON library.


DIR=$(dirname $0)
. $DIR/find-cmake-latest.sh
CMAKE=$(find_cmake_latest)
. $DIR/check-symlink.sh
SRCROOT=$(pwd)

# The major version of the project. Appears in certain install filenames.
_full_version=$(cat "$DIR/../../VERSION_CURRENT")
version="${_full_version%-*}"  # 1.2.3-dev → 1.2.3
major="${version%%.*}"         # 1.2.3     → 1
echo "major version: $major"
echo " full version: $version"

# Use ccache if able.
. $DIR/find-ccache.sh
find_ccache_and_export_vars "$(pwd)" || true

SCRATCH_DIR=$(pwd)/.scratch
rm -rf "$SCRATCH_DIR"
mkdir -p "$SCRATCH_DIR"
cp -r -- "$SRCROOT"/* "$SCRATCH_DIR"

if [ "$BSON_ONLY" ]; then
  BUILD_DIR=$SCRATCH_DIR/build-dir-bson
  INSTALL_PREFIX=$SCRATCH_DIR/install-dir-bson
else
  BUILD_DIR=$SCRATCH_DIR/build-dir-mongoc
  INSTALL_PREFIX=$SCRATCH_DIR/install-dir-mongoc
fi

if [ "$DESTDIR" ]; then
   INSTALL_DIR="${DESTDIR}/${INSTALL_PREFIX}"
else
   INSTALL_DIR=$INSTALL_PREFIX
fi

rm -rf $BUILD_DIR
mkdir $BUILD_DIR

rm -rf $INSTALL_DIR
mkdir -p $INSTALL_DIR

cd $BUILD_DIR

cp -r -- "$SRCROOT"/* "$SCRATCH_DIR"

if [ "$BSON_ONLY" ]; then
  BSON_ONLY_OPTION="-DENABLE_MONGOC=OFF"
else
  BSON_ONLY_OPTION="-DENABLE_MONGOC=ON"
fi

# Use ccache if able.
if [[ -f $DIR/find-ccache.sh ]]; then
  . $DIR/find-ccache.sh
  find_ccache_and_export_vars "$SCRATCH_DIR" || true
fi

$CMAKE -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX -DBUILD_TESTING=OFF -DCMAKE_PREFIX_PATH=$INSTALL_DIR/lib/cmake $BSON_ONLY_OPTION "$SCRATCH_DIR"
$CMAKE --build . --parallel
if [ "$DESTDIR" ]; then
   DESTDIR=$DESTDIR $CMAKE --build . --target install
else
   $CMAKE --build . --target install
fi
touch $INSTALL_DIR/lib/canary.txt

# no kms-message components should be installed
if test -f $INSTALL_DIR/include/kms_message/kms_message.h; then
  echo "kms_message.h found!"
  exit 1
else
  echo "kms_message.h check ok"
fi
if test -f $INSTALL_DIR/lib/libkms_message-static.a; then
  echo "libkms_message-static.a found!"
  exit 1
else
  echo "libkms_message-static.a check ok"
fi
if test -f $INSTALL_DIR/lib/cmake/kms_message/kms_message-config.cmake; then
  echo "kms_message-config.cmake found!"
  exit 1
else
  echo "kms_message-config.cmake check ok"
fi

ls -l $INSTALL_DIR/share/mongo-c-driver

$CMAKE --build . --target uninstall

set +o xtrace

if test -f $INSTALL_DIR/lib/pkgconfig/bson$major.pc; then
  echo "bson$major.pc found!"
  exit 1
else
  echo "bson$major.pc check ok"
fi
if test -f $INSTALL_DIR/lib/cmake/bson-$version/bsonConfig.cmake; then
  echo "bsonConfig.cmake found!"
  exit 1
else
  echo "bsonConfig.cmake check ok"
fi
if test -f $INSTALL_DIR/lib/cmake/bson-$version/bsonConfigVersion.cmake; then
  echo "bsonConfigVersion.cmake found!"
  exit 1
else
  echo "bsonConfigVersion.cmake check ok"
fi
if test ! -f $INSTALL_DIR/lib/canary.txt; then
  echo "canary.txt not found!"
  exit 1
else
  echo "canary.txt check ok"
fi
if test ! -d $INSTALL_DIR/lib; then
  echo "$INSTALL_DIR/lib not found!"
  exit 1
else
  echo "$INSTALL_DIR/lib check ok"
fi
if [ -z "$BSON_ONLY" ]; then
  if test -f $INSTALL_DIR/lib/pkgconfig/mongoc$major.pc; then
    echo "mongoc$major.pc found!"
    exit 1
  else
    echo "mongoc$major.pc check ok"
  fi
  if test -f $INSTALL_DIR/lib/cmake/mongoc-1.0/mongoc-1.0-config.cmake; then
    echo "mongoc-1.0-config.cmake found!"
    exit 1
  else
    echo "mongoc-1.0-config.cmake check ok"
  fi
  if test -f $INSTALL_DIR/lib/cmake/mongoc-1.0/mongoc-1.0-config-version.cmake; then
    echo "mongoc-1.0-config-version.cmake found!"
    exit 1
  else
    echo "mongoc-1.0-config-version.cmake check ok"
  fi
  if test -f $INSTALL_DIR/lib/cmake/mongoc-1.0/mongoc-targets.cmake; then
    echo "mongoc-targets.cmake found!"
    exit 1
  else
    echo "mongoc-targets.cmake check ok"
  fi
fi
if test -f $INSTALL_DIR/include/bson-$version/bson/bson.h; then
  echo "bson/bson.h found!"
  exit 1
else
  echo "bson/bson.h check ok"
fi
if test -d $INSTALL_DIR/include/bson-$version; then
  echo "$INSTALL_DIR/include/bson-$version found!"
  exit 1
else
  echo "$INSTALL_DIR/include/bson-$version check ok"
fi
if [ -z "$BSON_ONLY" ]; then
  if test -f $INSTALL_DIR/include/libmongoc-1.0/mongoc/mongoc.h; then
    echo "mongoc/mongoc.h found!"
    exit 1
  else
    echo "mongoc/mongoc.h check ok"
  fi
  if test -d $INSTALL_DIR/include/libmongoc-1.0; then
    echo "$INSTALL_DIR/include/libmongoc-1.0 found!"
    exit 1
  else
    echo "$INSTALL_DIR/include/libmongoc-1.0 check ok"
  fi
fi
if test -f $INSTALL_DIR/share/mongo-c-driver/uninstall-bson.sh; then
  echo "uninstall-bson.sh found!"
  exit 1
else
  echo "uninstall-bson.sh check ok"
fi
if test -f $INSTALL_DIR/share/mongo-c-driver/uninstall.sh; then
  echo "uninstall.sh found!"
  exit 1
else
  echo "uninstall.sh check ok"
fi
if test -d $INSTALL_DIR/share/mongo-c-driver; then
  echo "$INSTALL_DIR/share/mongo-c-driver found!"
  exit 1
else
  echo "$INSTALL_DIR/share/mongo-c-driver check ok"
fi
