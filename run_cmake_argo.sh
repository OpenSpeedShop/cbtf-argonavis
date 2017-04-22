#!/bin/bash

rm -rf  build
mkdir build
pushd build

export CC=`which gcc`
export CXX=`which g++`

export KRELL_ROOT=/u/glschult/OSS/osscbtf_v2.3.1
export DYNINST_ROOT=/u/glschultz/OSS/osscbtf_v2.3.1
export CBTF_ROOT=/u/glschult/OSS/osscbtf_v2.3.1
export CBTF_KRELL_ROOT=/u/glschult/OSS/osscbtf_v2.3.1
export CBTF_ARGO_PREFIX=/u/glschult/OSS/osscbtf_v2.3.1
export MY_CUDA_ROOT=/nasa/cuda/7.5
export BOOST_ROOT=/u/glschult/OSS/osscbtf_v2.3.1
export LIBMONITOR_DIR=/u/glschult/OSS/osscbtf_v2.3.1

export MY_CMAKE_PREFIX_PATH="${KRELL_ROOT}:${CBTF_ROOT}:${CBTF_KRELL_ROOT}"

cmake .. \
        -DCMAKE_BUILD_TYPE=None \
        -DCMAKE_CXX_FLAGS="-g -O2 -Wall" \
        -DCMAKE_C_FLAGS="-g -O2 -Wall" \
        -DCMAKE_VERBOSE_MAKEFILE=ON \
        -DCMAKE_INSTALL_PREFIX=${CBTF_ARGO_PREFIX} \
        -DCMAKE_PREFIX_PATH=${MY_CMAKE_PREFIX_PATH} \
        -DCBTF_DIR=${CBTF_ROOT} \
        -DCBTF_KRELL_DIR=${CBTF_KRELL_ROOT} \
        -DCUPTI_DIR=${MY_CUDA_ROOT}/extras/CUPTI \
        -DCUDA_INSTALL_PATH=${MY_CUDA_ROOT}\
        -DCUDA_DIR=${MY_CUDA_ROOT} \
        -DPAPI_DIR=${KRELL_ROOT} \
        -DMRNET_DIR=${KRELL_ROOT} \
	-DLIBMONITOR_DIR=${LIBMONITOR_DIR}

make clean
make
make install

