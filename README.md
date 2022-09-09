# 2 Ffast 2 Furious
## Or, The Narcissism of Small Differences

Despite my disclaimer that my earlier example ([The ffast and the Furious](https://gist.github.com/moyix/46b7d7fdb9e100dad866821793b08058)) of a program that becomes vulnerable in the presence of a shared library that was built with `-ffast-math`, some still found it too unrealistic.

This demo attempts to rectify that issue, thanks to [a nice idea from an1sotropy on Hacker News](https://news.ycombinator.com/item?id=32774694). The scenario is a program that plots a cool histogram of the differences between pairs of floating point numbers. Because the plot is drawn for the upper-left quadrant of a graph, the bin counts are reversed.

The core vulnerability comes from the *usually* very reasonable assumption that if `a != b`, then `abs(a-b) != 0.0`. However, when `a` and `b` are very close to one another, then their difference may be a subnormal/denormal number, *even if `a` and `b` are both normal floating point numbers*.  Thus, when a library linked with `crtfastmath.o` is loaded, enabling Flush to Zero, `abs(a-b)` is 0.0 even though `a != b`.

The actual vulnerability is very similar to the previous example: because `abs(a-b)` returns 0.0, `ceilf()` returns 0.0 instead of 1.0, which leads to reading and incrementing a bin outside of the range of valid bins.

## Building and Running

To build, just run `make`:

```
$ make
touch gofast.c
gcc -shared -fPIC -ffast-math -o gofast.so gofast.c
gcc -o 2_ffast_2_furious 2_ffast_2_furious.c -g -fsanitize=address -lm -ldl
```

Run it without any shared libraries loaded to see a beautiful histogram plot:

```
$ ./2_ffast_2_furious float_data.csv
        Histogram of abs(a-b) for 100 pairs
                                                  ▲ Ct
                    ▒▒                            ┃ 13
                    ▒▒                      ▒▒    ┃ 12
                    ▒▒                      ▒▒    ┃ 11
                    ▒▒                      ▒▒ ▒▒ ┃ 10
                    ▒▒ ▒▒                   ▒▒ ▒▒ ┃ 9
                    ▒▒ ▒▒                   ▒▒ ▒▒ ┃ 8
                 ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒          ▒▒ ▒▒ ┃ 7
                 ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒    ▒▒    ▒▒ ▒▒ ┃ 6
        ▒▒       ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒    ▒▒ ▒▒ ▒▒ ▒▒ ┃ 5
  ▒▒    ▒▒       ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒    ▒▒ ▒▒ ▒▒ ▒▒ ┃ 4
  ▒▒    ▒▒    ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ┃ 3
  ▒▒    ▒▒    ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ┃ 2
  ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ▒▒ ┃ 1
◀━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
  16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1
                    Difference
```

Now run it in the mode that loads `gofast.so`, and watch Address Sanitizer catch an out-of-bounds read (without address sanitizer, this would silently corrupt the heap metadata):

```
Loaded ./gofast.so
Fast math mode enabled! Gotta go fast!
DEBUG: Will write to bins[16]
=================================================================
==3140446==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x606000000
120 at pc 0x5561b89975bb bp 0x7ffcaef034e0 sp 0x7ffcaef034d0
READ of size 4 at 0x606000000120 thread T0
    #0 0x5561b89975ba in add_to_bin /home/moyix/git/2_ffast_2_furious/2_ffast_2
_furious.c:42
    #1 0x5561b8998264 in main /home/moyix/git/2_ffast_2_furious/2_ffast_2_furio
us.c:182
    #2 0x7ffa52f97082 in __libc_start_main ../csu/libc-start.c:308
    #3 0x5561b89973ed in _start (/home/moyix/git/2_ffast_2_furious/2_ffast_2_fu
rious+0x23ed)

0x606000000120 is located 0 bytes to the right of 64-byte region [0x6060000000e
0,0x606000000120)
allocated by thread T0 here:
    #0 0x7ffa533c7a06 in __interceptor_calloc ../../../../src/libsanitizer/asan
/asan_malloc_linux.cc:153
    #1 0x5561b8998167 in main /home/moyix/git/2_ffast_2_furious/2_ffast_2_furio
us.c:180
    #2 0x7ffa52f97082 in __libc_start_main ../csu/libc-start.c:308

SUMMARY: AddressSanitizer: heap-buffer-overflow /home/moyix/git/2_ffast_2_furio
us/2_ffast_2_furious.c:42 in add_to_bin
Shadow bytes around the buggy address:
  0x0c0c7fff7fd0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x0c0c7fff7fe0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x0c0c7fff7ff0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x0c0c7fff8000: fa fa fa fa fd fd fd fd fd fd fd fa fa fa fa fa
  0x0c0c7fff8010: fd fd fd fd fd fd fd fd fa fa fa fa 00 00 00 00
=>0x0c0c7fff8020: 00 00 00 00[fa]fa fa fa fa fa fa fa fa fa fa fa
  0x0c0c7fff8030: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x0c0c7fff8040: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x0c0c7fff8050: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x0c0c7fff8060: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x0c0c7fff8070: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
Shadow byte legend (one shadow byte represents 8 application bytes):
  Addressable:           00
  Partially addressable: 01 02 03 04 05 06 07
  Heap left redzone:       fa
  Freed heap region:       fd
  Stack left redzone:      f1
  Stack mid redzone:       f2
  Stack right redzone:     f3
  Stack after return:      f5
  Stack use after scope:   f8
  Global redzone:          f9
  Global init order:       f6
  Poisoned by user:        f7
  Container overflow:      fc
  Array cookie:            ac
  Intra object redzone:    bb
  ASan internal:           fe
  Left alloca redzone:     ca
  Right alloca redzone:    cb
  Shadow gap:              cc
==3140446==ABORTING
```

## Note for macOS Users

On macOS, `gcc` is usually a symlink to `clang`. As far as I can tell, on macOS `clang` does not link to `crtfastmath.o` (unlike on Linux, where it links to `crtfastmath.o` if it finds one from an existing `gcc` installation). If you have `gcc` installed from `homebrew`, you can *try* using the real `gcc` by building with `make CC=/path/to/gcc` (I haven't gotten this to work because on my system `gcc` fails to find `-lSystem`).
