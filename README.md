twtclt
======

An attempt at writing a twitter client in C.

### Compiling

You should first find **libcurl**, **liboauth** and **libjson-c** in your distribution's repositories. These libraries are definitely available on Debian and Arch Linux.

However, **libmojibake** has to be compiled manually. Before compiling **twtclt**, compile **libmojibake**: 

    $ cd libmojibake
    $ make

After doing so, **twtclt** will be able to statically link **libmojibake**. Right now, compiling **twtclt** only takes two commands:

    $ cmake .
    $ make

As a result, the executable file **twtclt** will appear in the directory.

You can also install **twtclt** globally by typing

    # make install
    
as root (or using sudo).

**TODO:** correct installation procedures, automatic **libmojibake** compilation.

### Libraries

This application uses the following libraries:

* [libjson-c](https://github.com/json-c/json-c) for JSON parsing. \[MIT\]
* [liboauth](http://liboauth.sourceforge.net/) for OAuth authentication. \[MIT\]
* [libcurl](http://curl.haxx.se/) for HTTP(S) support. \["MIT/X derivate license"\]
* [libmojibake](https://github.com/JuliaLang/libmojibake) for UTF-8 parsing. \[MIT\]
* [ncursesw](https://www.gnu.org/software/ncurses/) for a console UI. \[X11\]
