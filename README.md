# LLVM for NEC SX-Aurora VE

This is a fork of the LLVM repositoy with experimental support for the NEC
SX-Aurora TSUBASA Vector Engine (VE).

### Features

- C, C++ support.
- Automatic vectorization through LLVM's loop and SLP vectorizers.
- Preview release of OpenMP target offloading in C code from the VH to VE. This
  prototype has been developed by RWTH Aachen.
- VEL intrinsics for low-level vector programming.
- This release is bundled with the Region Vectorizer for outer-loop
  vectorization with arbitrary nested control flow.

### Build instructions

To build llvm-ve from source refer to 
[llvm-dev](https://github.com/sx-aurora-dev/llvm-dev) and
[Compile.rst](llvm/docs/VE/Compile.rst).

### General Usage

To compile for the VE run Clang/Clang++ with the following command line:

    $ /opt/nec/nosupport/llvm-ve/clang -target ve-linux -O3 ...

### Outer-loop vectorization (Region Vectorizer)

The Region Vectorizer ([RV](https://github.com/cdl-saarland/rv)) provides
advanced vectorization for outer-loop and whole-functions in LLVM through user
annotations.  RV is activated by running 'rvclang(++)' instead of 'clang(++)'.
To trigger RV on a loop use a vectorization pragma (eg annotate the loop with
`#pragma omp simd` and pass the compiler option `-fopenmp-simd`).  RV is
available on both the VE and the VH.

### OpenMP target offloading (preview feature)

The OpenMP target offloading feature requires that 'clang' is in your PATH. 
To enable the OpenMP target offloading feature for C programs use:

    $ /opt/nec/nosupport/llvm-ve/clang \
      -fopenmp -fopenmp-targets=aurora-nec-veort -O3 ...

The implementation invokes another compiler instance to compile the offloaded
code for the VE.  By default, this is 'clang' as found in your PATH. To choose
a different target compiler, pass the option '-fopenmp-nec-compiler=COMPILER'
where 'COMPILER' may be either of 'clang', to use llvm-ve, 'rvclang' to use
llvm-ve with the region vectorizer or 'ncc', to use the NEC C Compiler at
'/opt/nec/ve/bin/ncc' or an absolute path to a target compiler by using the
prefix `path:`, ie

    -fopenmp-nec-compiler=path:/opt/nec/ve/bin/ncc-3.0.4

The default target compiler can be configured by changing the value of the
CMake variable ``NECAURORA_DEFAULT_TARGET_COMPILER``.

To pass additional command line arguments to the target compiler, use the flag
`-Xopenmp-target <argv_to_pass_on>` repeatedly for each argument.

### VEL Intrinsics for direct vector programming

See [the manual](https://sx-aurora-dev.github.io/velintrin.html).  To use VEL
intrinsics, pass the compiler option `-mattr=+velintrin`.  The resulting LLVM
bitcode and objects are compatible with those compiler without this option.
