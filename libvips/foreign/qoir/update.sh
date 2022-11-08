#!/bin/bash
set -e

[ -d qoir ] || git clone https://github.com/nigeltao/qoir

echo copying out source files ...

cp qoir/src/qoir.h .

if [ -d "patches" ]
then
  echo applying patches ...
  for patch in patches/*.patch; do
    patch -p0 <$patch
  done
fi

echo cleaning up ...
rm -rf qoi
