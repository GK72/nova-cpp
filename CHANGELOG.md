# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Feat

- `nova::error` has an optional `category` and `detail`; `nova::wrap_error(category, message, detail)`
  returns one for `expected`. A message-only error formats as before.

### Refact

- Adding `.hh` header files.

### Deprecation

- `.h` header files.
