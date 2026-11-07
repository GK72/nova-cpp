# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.13.0]

### Added

- `nova::ring<T>` (`ring.hpp`), a ring buffer iterated from the oldest element to
  the newest.

## [0.12.1]

### Fixed

- `code_analysis()` applies the warnings with the given visibility. They were
  always `INTERFACE`: `code_analysis(<target> PRIVATE)` left the sources of the
  target out, and the installed package passed the warnings and `-Werror` on to
  its consumers.

### Changed

- `enable_compiler_warnings()` requires a visibility, the same as
  `code_analysis()`. For the old behaviour, pass `INTERFACE`.

## [0.12.0]

`nova::dyn_value` is the one interface to YAML, JSON and TOML documents.

The formats are internal (`libnova/detail/`): nlohmann_json and yaml-cpp are no
longer public dependencies. Consumers need neither header; the static library
needs yaml-cpp (0.9.0) for linking.

### Added

- `libnova/dyn_value.hpp`
- `nova::refl` (`refl.hpp`): enum names and annotations by reflection, with
  `NOVA_EXPERIMENTAL_FEATURE_SET` (`-freflection`, `-fcontracts`).

### Removed

- `nova::json` and `nova::yaml` (`json.hpp`, `yaml.hpp`) are removed; use
  `nova::dyn_value`.

## [0.11.0]

Redesigned version information with added build information.

### Added

- CMake module `novaBuildInfo` for downstream consumers.
- Added User Guide and Developer Manual.

### Changed

- `nova::library_version()` returns a `std::string_view` without the `v` prefix
  (`0.10.0`).

### Removed

- `NovaVersionMajor`, `NovaVersionMinor`, and `NovaVersionPatch` are removed.

### Misc

- The version lives in `libnova/version.txt` only, CMake, the Conan recipe and
  `version-bump.sh` read it.
- Installed CMake helpers moved from `cmake/nova/details` to `cmake/nova/detail`.

## [0.10.0]

Redesigned error handling API.

### Added

- `nova::error_code_traits<E>` turns any enum into an error origin with a name
  and messages (and optionally `std::errc` conditions); nova generates the
  `std::error_category`.
- `expected::transform` and `expected::transform_error` map the value or the
  error, as `std::expected` does.

### Changed

- `nova::error` is a `std::error_code` and a `detail`, formatted as `[category]
  message (detail)`; `nova::wrap_error(code, detail)` returns one for
  `expected`. Categories are per origin.
- `nova::exception` moved to `exception.hpp` (`error.hpp` still includes it).
  `wrap_error` moved from `expected.hpp` to `error.hpp`, which now depends on
  `expected.hpp`.
- `read_file` and `read_bin` report why a path is not a regular file;
  `set_cpu_affinity` reports `errno`.
- `parse_error` and the new `env_error` are error enums (`parse` and `env`
  categories) mapped to `std::errc` conditions. `parse_error` values start from
  one.

### Removed

- `nova::error::message` and constructing a `nova::error` from a string are
  removed; use a code and a detail instead.
