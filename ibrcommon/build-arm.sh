crossroot="/home/yael/dev/raspi/staging/armv6-rpi-linux-gnueabihf"
sysroot="${crossroot}/armv6-rpi-linux-gnueabihf/sysroot"
crosspath="${crossroot}/bin/armv6-rpi-linux-gnueabihf-"
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
./configure --host=arm-linux-gnueabihf
make -j7
make install DESTDIR=$sysroot
