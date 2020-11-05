crossroot="/home/yael/x-tools/arm-rpi-linux-gnueabihf"
sysroot="${crossroot}/arm-rpi-linux-gnueabihf/sysroot"
crosspath="${crossroot}/bin/arm-rpi-linux-gnueabihf-"
export CC="${crosspath}gcc"
export CXX="${crosspath}g++"
export AR="${crosspath}ar"
export LD="${crosspath}ld"
export OBJCOPY="${crosspath}objcopy"
export OBJDUMP="${crosspath}objdump"
export NM="${crosspath}nm"
export RANLIB="${crosspath}ranlib"
export CPPFLAGS="-I${sysroot}/usr/local/include"
#export LDFLAGS="-L${sysroot}/lib"
#export PKG_CONFIG_PATH=
#export PKG_CONFIG_LIBDIR="${sysroot}/usr/local/lib/pkgconfig"
#export PKG_CONFIG_SYSROOT_DIR=$sysroot
./configure --host=arm-rpi-linux-gnueabihf
make -j7
make install DESTDIR=$sysroot
