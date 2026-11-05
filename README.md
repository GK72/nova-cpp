# Nova Library

![Build](https://github.com/GK72/nova-cpp/actions/workflows/build.yml/badge.svg)

A collection of modern C++ code following modern and pedantic C++ with clean,
intuitive APIs. Also a place for testing the newest features and libraries.

# Coding and styleguide

The commits are following the [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/).

# Dependencies

See `libnova/conandata.yml` for more details.

# Quick-start

The CI pipeline runs locally on a fresh clone of the committed `HEAD` (in
`/tmp/workspace/nova`), the same stages as the GitHub workflows:

```sh
CMAKE_BUILD_PARALLEL_LEVEL=4 ci/e2e.sh
```

<!-- Use the following command to build with experimental features. -->

<!-- ```sh -->
<!-- CMAKE_BUILD_PARALLEL_LEVEL=8 ./run -s build-libnova-exp -->
<!-- ``` -->

A single stage runs on the checkout itself, e.g. `ci/build.sh`. `BUILD_TYPE`,
`SANITIZERS` and `COVERAGE` are configurable, see `ci/common.sh`.

# Project structure

The project follows the [Canonical Project Structure](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1204r0.html).

```
├── ci                                          CI stages, `e2e.sh` runs all of them locally
├── env                                         Provided environment for developing/testing
│   └── suse.Dockerfile                         For testing RPM packaging
├── examples
├── libnova
│   ├── cmake                                   Public CMake modules
│   └── libnova                                 C++ source code
│       ├── detail                              Internal header files
│       ├── *.test.cpp                          Unit tests
│       └── *.bench.cpp                         Benchmarks
├── scripts                                     Helpers for developing/testing the project
└── tests                                       Integration/functional tests
    ├── <test-suite>                            Functional test suite
    └── package                                 Package testing for Conan workflow
```
