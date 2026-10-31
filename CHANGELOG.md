# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Experimental]

### Feat

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
- `expected::transform` and `expected::transform_error` map the value or the
  error, as `std::expected` does.
- `nova::dyn_value` is the one interface to YAML, JSON and TOML documents:
  `parse`, `read_from` and `read_config` read them (the format by the file
  extension), `dump` and `write_to` write them, `lookup`, `find`, `is` and
  `at` query them by `dyn_path`, and `merge` layers them (a null deletes a
  key). Lookups are strict: a wrong type throws, naming the path.
- `nova::refl` (`refl.hpp`): enum names and annotations by reflection, with
  `NOVA_EXPERIMENTAL_FEATURE_SET` (`-freflection`, `-fcontracts`).

### Breaking

- `nova::error::message` and constructing a `nova::error` from a string are
  removed; use a code and a detail instead.
- `nova::exception` moved to `exception.hpp` (`error.hpp` still includes it).
  `wrap_error` moved from `expected.hpp` to `error.hpp`, which now depends on
  `expected.hpp`.
- `nova::json` and `nova::yaml` (`json.hpp`, `yaml.hpp`) are removed; use
  `nova::dyn_value`. The formats are internal (`libnova/detail/`): nlohmann_json
  and yaml-cpp are no longer public dependencies. Consumers need neither
  header; the static library needs yaml-cpp (0.9.0) for linking.
- `libnova/details/` is renamed to `libnova/detail/`, matching the
  `nova::detail` namespace; so is `lib/cmake/nova/details/` of the installed
  package.
