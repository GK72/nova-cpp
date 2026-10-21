# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Experimental]

### Feat

- Executors: `nova::task` (a move-only callable), `nova::executor`, `nova::thread_pool`,
  `nova::strand` (one task at a time, in order, on another executor's threads),
  `nova::inline_executor` and `nova::background()`, the process's pool for background work.

- `nova::error` is a `std::error_code` and a `detail`, formatted as `[category]
  message (detail)`; `nova::wrap_error(code, detail)` returns one for
  `expected`. Categories are per origin.
- `nova::error_code_traits<E>` turns any enum into an error origin with a name
  and messages (and optionally `std::errc` conditions); nova generates the
  `std::error_category`.
- `parse_error` and the new `env_error` are error enums (`parse` and `env`
  categories) mapped to `std::errc` conditions. `parse_error` values start from
  one.
- `read_file` and `read_bin` report why a path is not a regular file;
  `set_cpu_affinity` reports `errno`.

### Breaking

- `nova::error::message` and constructing a `nova::error` from a string are
  removed; use a code and a detail instead.
- `nova::exception` moved to `exception.hpp` (`error.hpp` still includes it).
  `wrap_error` moved from `expected.hpp` to `error.hpp`, which now depends on
  `expected.hpp`.
