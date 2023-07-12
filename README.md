# stunpack

**Stunts/4D [Sports] Driving game resource unpacker**

This program decodes packed resources and code files used by the PC version of the game "Stunts" (Brøderbund), also published as "4D Sports Driving" (Mindscape) and "4D Driving" (Electronic Arts).

An encoder has not been made as the game accepts uncompressed resource files.

## Download

Pre-compiled binaries can be downloaded from the project's [release page](https://github.com/dstien/stunpack/releases).

## Usage

The program requires at least one parameter as the source file path. If the optional destination file path is omitted a new filename is generated by adding the `.out` extension to the source file name. Except for the DOS version, where the last character of the source file name is replaced by a `_` in the destination file name.

For a full list of options run `stunpack -h`.

## Building

The project can be compiled with the GNU toolchain by running `make`. Building for other targets can be achieved by setting a compiler/linker in the `CC` environment variable:
* Win32 with MinGW: `CC=i686-w64-mingw32-gcc make`
* Win64 with MinGW: `CC=x86_64-w64-mingw32-gcc make`
* MS DOS with Open Watcom: `CC=wcl386 INCLUDE=$WATCOM/h LIB=$WATCOM/lib386 PATH=$WATCOM/binl:$WATCOM/binw:$PATH make`
* Any target exposed by Zig's Clang interface: `CC="zig cc -target riscv64-linux-musl" make`

Variables that affects the build process:
* `CC`: Compiler executable
* `CFLAGS`: Compiler flags
* `LDFLAGS`: Linker flags
* `BUILDDIR`: Place output files in external directory
* `EXESUFFIX`: Defaults to `.exe` if a Windows or DOS compiler is detected
* `INSTALLDIR`: Defaults to `/usr/local/bin` for `make install`

## Additional documentation

The data format, compression algorithms and applied optimisations are described in the [Stunts Wiki](https://wiki.stunts.hu/wiki/Compression).

## License

GNU GPL version 2, see COPYING.

## Contact

* daniel@stien.org
* https://github.com/dstien/stunpack
