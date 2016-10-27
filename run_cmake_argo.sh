rm -rf  build
mkdir build
pushd build

export CC=`which gcc`
export CXX=`which g++`

export KRELL_ROOT=/opt/DEVEL/krellroot_v2.2.3
export DYNINST_ROOT=/opt/DEVEL/krellroot_v2.2.3
export CBTF_ROOT=/opt/DEVEL2/cbtf_v2.2.4
export CBTF_KRELL_ROOT=/opt/DEVEL2/cbtf_v2.2.4
export CBTF_ARGO_PREFIX=/opt/DEVEL2/cbtf_v2.2.4
export MY_CUDA_ROOT=/usr/local/cuda-7.5

#./install-cbtf --install-prefix /opt/DEVEL/cbtf_only_v1.1 \
#               --with-cbtf-root /opt/DEVEL/cbtf_only_v1.1 \
#               --with-libdwarf-root /opt/krellroot_v2.2.2 \
#               --with-libunwind-root /opt/krellroot_v2.2.2 \
#               --with-papi-root /opt/krellroot_v2.2.2 \
#               --with-libmonitor-root /opt/krellroot_v2.2.2 \
#               --with-dyninst-root /opt/krellroot_v2.2.2 \
#               --with-mrnet-root /opt/krellroot_v2.2.2 \
#               --with-xercesc-root  /opt/krellroot_v2.2.2 \
#               --with-cuda /usr/local/cuda-7.5 \
#               --with-cupti /usr/local/cuda-7.5/extras/CUPTI \
#               --with-openmpi /opt/openmpi-1.10.2

export MY_CMAKE_PREFIX_PATH="${KRELL_ROOT}:${CBTF_ROOT}:${CBTF_KRELL_ROOT}"
cmake .. \
        -DCMAKE_BUILD_TYPE=None \
        -DCMAKE_CXX_FLAGS="-g -O2" \
        -DCMAKE_C_FLAGS="-g -O2" \
        -DCMAKE_VERBOSE_MAKEFILE=ON \
        -DCMAKE_INSTALL_PREFIX=${CBTF_ARGO_PREFIX} \
        -DCMAKE_PREFIX_PATH=${MY_CMAKE_PREFIX_PATH} \
        -DCBTF_DIR=${CBTF_ROOT} \
        -DCBTF_KRELL_DIR=${CBTF_KRELL_ROOT} \
        -DCUPTI_DIR=${MY_CUDA_ROOT}/extras/CUPTI \
        -DCUDA_INSTALL_PATH=${MY_CUDA_ROOT}\
        -DCUDA_DIR=${MY_CUDA_ROOT} \
        -DPAPI_DIR=${KRELL_ROOT} \
        -DMRNET_DIR=${KRELL_ROOT}

#cmake .. \
#        -DCMAKE_BUILD_TYPE=None \
#        -DCMAKE_CXX_FLAGS="-g -O2" \
#        -DCMAKE_C_FLAGS="-g -O2" \
#	-DCMAKE_VERBOSE_MAKEFILE=ON \
#	-DCMAKE_INSTALL_PREFIX=${CBTF_ARGO_PREFIX} \
#	-DCMAKE_PREFIX_PATH=${MY_CMAKE_PREFIX_PATH} \
#	-DCBTF_DIR=${CBTF_ROOT} \
#	-DCBTF_KRELL_DIR=${CBTF_KRELL_ROOT} \
#	-DPAPI_DIR=${KRELL_ROOT} \
#	-DCUPTI_DIR=${MY_CUDA_ROOT}/extras/CUPTI \
#	-DMRNET_DIR=${KRELL_ROOT} 
	

make clean
make
make install

