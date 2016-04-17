# The PATOS source-to-source compiler

This software is part of the research paper "F. Richter-Gottfried, P. Kreutzer, A. Ditter, M. Schneider, D. Fey: C++ Classes and Templates for OpenCL Kernels with PATOS".

## Dependencies

PATOS requires (at least) the following libraries and tools:

- `libclang` and `clang` (tested with version `3.5`)
- `libllvm` and `llvm` (tested with version `3.5`)
- `libboost`
- `libzdev`
- `libedit`

## Building and running the application

To build PATOS type

```
make
```

in the root directory.

To run the application (on a Unix system), use the script `patos.sh` in the root directory.

## Example

See the directory `sorting_test` for an example of a program that can be translated with PATOS. Use the script `compile_sorting_test.sh` to translate the example.

## License

PATOS uses the ISC license (see `LICENSE` for more information).
