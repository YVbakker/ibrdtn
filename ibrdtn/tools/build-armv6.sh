host_id="armv6-rpi-linux-gnueabihf"
crossroot="/home/yael/x-tools/${host_id}"
sysroot="${crossroot}/${host_id}/sysroot"
crosspath="${crossroot}/bin/${host_id}-"
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
./configure --host=$host_id
make clean
make -j7
make install DESTDIR=$sysroot
