# Nova Library

![Build](https://github.com/GK72/nova-cpp/actions/workflows/build.yml/badge.svg)

A collection of modern C++ code following modern and pedantic C++ with clean,
intuitive APIs. Also a place for testing the newest features and libraries.

# Coding and styleguide

The commits are following the [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/).

# Dependencies

See `libnova/conandata.yml` for more details.

# Quick-start

A runner script is provided at the root of the project for verification as a
CI-like workflow. It runs all stages for `Debug` and `Release` build by
default.

```sh
CMAKE_BUILD_PARALLEL_LEVEL=4 ./run [--help]
```

# Project structure

The project follows the [Canonical Project Structure](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1204r0.html).

```
├── env                                         Provided environment for developing/testing
│   └── suse.Dockerfile                         For testing RPM packaging
├── examples
├── libnova
│   ├── cmake                                   Public CMake modules
│   └── libnova                                 C++ source code
│       ├── details                             Internal header files
│       ├── *.test.cpp                          Unit tests
│       └── *.bench.cpp                         Benchmarks
├── scripts                                     Helpers for developing/testing the project
└── tests                                       Integration/functional tests
    ├── <test-suite>                            Functional test suite
    └── package                                 Package testing for Conan workflow
```
