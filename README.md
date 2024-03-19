# Nova Library

A collection of modern C++ code following modern and pedantic C++ with clean,
intuitive APIs. Also a place for testing the newest features and libraries.

# Coding and styleguide

The commits are following the [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/).

# Dependencies

# Building the library

The library is developed by the latest and greatest tools on Arch Linux.
Currently, there are no guarantees in which environment it works. The general
aim is to be cross-platform working on any major compiler vendor.

# Project structure

Under migration to [Canonical Project Structure].

```
├── cmake
├── doc
├── include                 Public headers (legacy).
├── libnova                 Source code for general library.
├── libnova-gfx             Source code for graphics library.
├── nova                    Source code for executables.
├── unit-tests              (legacy)
└── functional-tests        (legacy)
```

[Canonical Project Structure][https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1204r0.html]
