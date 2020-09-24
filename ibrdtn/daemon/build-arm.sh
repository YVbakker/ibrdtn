crossroot="/home/yael/x-tools/arm-unknown-linux-gnueabi"
sysroot="${crossroot}/arm-unknown-linux-gnueabi/sysroot"
crosspath="${crossroot}/bin/arm-unknown-linux-gnueabi-"
export CC="${crosspath}gcc --sysroot=${sysroot}"
export CXX="${crosspath}g++ --sysroot=${sysroot}"
export AR="${crosspath}ar"
export LD="${crosspath}ld"
export OBJCOPY="${crosspath}objcopy"
export OBJDUMP="${crosspath}objdump"
export NM="${crosspath}nm"
export RANLIB="${crosspath}ranlib"
export PKG_CONFIG_PATH=
export PKG_CONFIG_LIBDIR="${sysroot}/usr/local/lib/pkgconfig"
export PKG_CONFIG_SYSROOT_DIR=$sysroot
./configure --host=arm-linux-gnueabi
make -j7
make install DESTDIR=$sysroot
