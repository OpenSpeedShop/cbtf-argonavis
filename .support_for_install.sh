#!/bin/bash 
#
# Set up we find the autotools for bootstrapping
#
set -x
export bmode=""
if [ `uname -m` = "x86_64" -o `uname -m` = " x86-64" ]; then
    LIBDIR="lib64"
    ALTLIBDIR="lib"
    echo "UNAME IS X86_64 FAMILY: LIBDIR=$LIBDIR"
    export LIBDIR="lib64"
elif [ `uname -m` = "ppc64" ]; then
   if [ $CBTF_PPC64_BITMODE_32 ]; then
    LIBDIR="lib"
    ALTLIBDIR="lib64"
    echo "UNAME IS PPC64 FAMILY, but 32 bitmode: LIBDIR=$LIBDIR"
    export LIBDIR="lib"
    export bmode="--with-ppc64-bitmod=32"
   else
    LIBDIR="lib64"
    ALTLIBDIR="lib"
    echo "UNAME IS PPC64 FAMILY, with 64 bitmode: LIBDIR=$LIBDIR"
    export LIBDIR="lib64"
    export CFLAGS=" -m64 $CFLAGS "
    export CXXFLAGS=" -m64 $CXXFLAGS "
    export CPPFLAGS=" -m64 $CPPFLAGS "
    export bmode="--with-ppc64-bitmod=64"
   fi
elif [ `uname -m` = "ppc" ]; then
    LIBDIR="lib"
    ALTLIBDIR="lib64"
    echo "UNAME IS PPC FAMILY: LIBDIR=$LIBDIR"
    export LIBDIR="lib"
    export bmode="--with-ppc64-bitmod=32"
else
    LIBDIR="lib"
    ALTLIBDIR="lib64"
    export LIBDIR="lib"
    echo "UNAME IS X86 FAMILY: LIBDIR=$LIBDIR"
fi

#sys=`uname -n | grep -o '[^0-9]\{0,\}'`
sys=`uname -n `
export MACHINE=$sys
echo ""
echo '    machine: ' $sys
                                                                                    

if [ -z "$CBTF_MPI_MPICH2" ]; then
 export CBTF_MPI_MPICH2=/usr
fi

if [ -z "$CBTF_MPI_MVAPICH" ]; then
 export CBTF_MPI_MVAPICH=/usr
fi

if [ -z "$CBTF_MPI_MVAPICH2" ]; then
 export CBTF_MPI_MVAPICH2=/usr
fi

if [ -z "$CBTF_MPI_MPT" ]; then
 export CBTF_MPI_MPT=/usr
fi

if [ -z "$CBTF_MPI_OPENMPI" ]; then
 export CBTF_MPI_OPENMPI=/usr
fi


# you may want to load the latest autotools for the bootstraps to succeed.
echo "-------------------------------------------------------------"
echo "-- START BUILDING CBTF  -------------------------------------"
echo "-------------------------------------------------------------"

if [ -z "$CUDA_INSTALL_PATH" ]; then
  echo "*** NOTE *** If you wish to build the cuda collector and components please set:"
  echo "*** NOTE *** CUDA_INSTALL_PATH to the installation directory path for the cuda toolkit."
  echo "*** NOTE *** CUPTI_ROOT to the installation directory path for the cupti components."
  echo "*** NOTE *** For example: CUPTI_ROOT=/usr/local/cuda-5.0/extras/CUPTI"
  echo "*** NOTE *** For example: CUDA_INSTALL_PATH=/usr/local/cuda-5.0"

else

  echo "-------------------------------------------------------------"
  echo "-- BUILDING CUDA --------------------------------------------"
  echo "-------------------------------------------------------------"

  cd CUDA

  if [ ! -d "build" ]; then
      mkdir build
  fi

  cd build

  export CC=`which gcc`
  export CXX=`which c++`
  export CPLUSPLUS=`which c++`

  cmake -DCMAKE_MODULE_PATH=$CBTF_PREFIX/share/KrellInstitute/cmake -DCMAKE_INSTALL_PREFIX=$CBTF_PREFIX -DCBTF_DIR=$CBTF_PREFIX -DCBTF_KRELL_DIR=$CBTF_PREFIX ..

  make

  echo "-------------------------------------------------------------"
  echo "-- INSTALLING CUDA ------------------------------"
  echo "-------------------------------------------------------------"

  make install

  if [ -f $CBTF_PREFIX/$LIBDIR/libcbtf-messages-cuda.so -a -f $CBTF_PREFIX/$LIBDIR/KrellInstitute/Collectors/cuda-collector-monitor-mrnet-mpi.so ]; then
     echo "CBTF CUDA BUILT SUCCESSFULLY into $CBTF_PREFIX."
  else
     echo "CBTF CUDA FAILED TO BUILD - TERMINATING BUILD SCRIPT.  Please check for errors."
     exit
  fi

fi


echo "-------------------------------------------------------------"
echo "-- END OF BUILDING CBTF  ------------------------------------"
echo "-------------------------------------------------------------"
echo "-------------------------------------------------------------"
echo "-- END OF BUILDING CBTF  ------------------------------------"
echo "-------------------------------------------------------------"
