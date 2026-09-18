# Contributing

geome is licensed under the MIT License. Contributions are accepted under that
license. See [LICENSE](LICENSE).

## Build and Test

Install the dependencies listed in README.md, then run:

```sh
make clean
make check CFLAGS='-O2 -g -Werror'
make clean
make check CC=clang CFLAGS='-O2 -g -Werror'
make sanitize CC=clang CFLAGS='-O1 -g -Werror'
```

Tests must be offline and deterministic by default. Do not enable live tests
in pull requests or CI. Never attach real IP addresses or location results to
bug reports. Use the documentation-only fixtures and sanitized diagnostics.

## Style and Review

Use ISO C11, four-space indentation, braces on their own line for functions,
and descriptive names. Keep the existing module boundaries and public output
contract. Avoid unrelated formatting. Compile with all Makefile warnings and
`-Werror`; there is no additional formatter requirement.

Every allocation and library operation must have a checked failure path. Add
fixture tests for behavior changes and allocation tests when ownership changes.
Use cJSON for JSON, libcurl for HTTPS, and no subprocesses in production.
Keep CLI help, man page, README, and exit-code tests consistent.

Pull requests should describe the bug or goal, the narrow implementation,
offline validation commands and results, and any privacy or packaging impact.
Run `dpkg-buildpackage -us -uc -b` and lintian for packaging changes. Do not
silence findings without a documented reason. New providers, telemetry, caches,
endpoint configuration, licenses, and namespace changes require owner review.
