#!/bin/bash
basepath=$(cd `dirname $0`; pwd)
cd ${basepath}/3rd
chmod +x build.sh
./build.sh

cd ${basepath}/src
make

cp ${basepath}/src/transtools ${basepath}
