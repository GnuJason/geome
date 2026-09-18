# Build Validation and Delivery

## Recommended Installation Methods

1. **Debian package (.deb):** officially provided for compatible amd64 systems.
2. **APT repository (future):** no project APT repository is currently available.
3. **Source tarball build:** an official, first-class installation method,
   including for Debian 12 and LMDE.
4. **RPM package (future):** planned; no RPM release artifact is currently available.

## Supported Platforms

| Platform | Verified result or support scope |
| --- | --- |
| Debian 12 | Project-owner-reported source-tarball installation on a Debian 12/LMDE system |
| Linux Mint Debian Edition (LMDE) | Project-owner-reported source build on the same system; not a separate container test |
| Debian 13 (Trixie), amd64 | Clean package build, offline suite, and installation validated locally in Docker |
| Ubuntu 24.04, amd64 | Published package installed; version/help and one live JSON request validated locally in Docker |
| Other Ubuntu-family distributions | Use the `.deb` only with compatible architecture and dependencies; not individually validated |
| General Linux distributions | Source installation supported with the required toolchain and development libraries; not universally tested |

The Debian 12/LMDE result is an owner-reported source installation, not proof
that the published `.deb` works on Debian 12. That `.deb` was built on Debian 13
and depends on `libcurl4t64`; Debian 12 supplies `libcurl4`. Use a source build
on Debian 12 rather than forcing the package dependencies. Compatibility on
Ubuntu-family systems depends on their base release. No Zorin test is claimed.

## Debian Package Installation (.deb)

Get the package from [geome v1.0.0](https://github.com/GnuJason/geome/releases/tag/v1.0.0).
No Git installation or source checkout is needed:

```sh
wget https://github.com/GnuJason/geome/releases/download/v1.0.0/geome_1.0.0-1_amd64.deb
sudo apt update
sudo apt install ./geome_1.0.0-1_amd64.deb
geome --version
```

Use the release-note SHA-256 checksums to verify downloads. The package includes
`/usr/bin/geome` and `/usr/share/man/man1/geome.1.gz`. Minimal Ubuntu container
images may exclude man pages via `path-exclude=/usr/share/man/*`; this was
confirmed as an environment limitation, not a missing package payload.

## Build From Source Tarball

The project owner has successfully validated this official installation method
on a Debian 12 / Linux Mint Debian Edition system. Install dependencies first:

```sh
sudo apt update
sudo apt install build-essential pkg-config \
  libcurl4-openssl-dev libcjson-dev
```

Download using `wget` (or a browser), then build and install:

```sh
wget https://github.com/GnuJason/geome/releases/download/v1.0.0/geome-1.0.0.tar.gz
tar -xvf geome-1.0.0.tar.gz
cd geome-1.0.0
make
sudo make install
geome --version
```

Compilation runs as an ordinary user. Only installation into the default
`/usr/local` prefix requires sudo. Ensure `/usr/local/bin` is on your PATH.
Use `sudo make uninstall` from the extracted directory to remove that install;
user-local and staged alternatives are documented in README.md.

Other Linux distributions require equivalent C11, GNU Make, pkg-config, libcurl,
and cJSON development packages and a trusted CA store. If `libcjson-dev` is
unavailable, install cJSON separately using the distribution's equivalent package
or [upstream source instructions](https://github.com/DaveGamble/cJSON#building).
cJSON must be at least 1.7.13, with headers, a linkable library, and `libcjson.pc`
available. For custom prefixes, configure `PKG_CONFIG_PATH` and the runtime
linker's search path as needed.

### Missing cJSON Headers

For this compiler error:

```text
fatal error: cjson/cJSON.h: No such file or directory
```

Install the cJSON development package, for example
`sudo apt install libcjson-dev`, or build and install cJSON from source as above.
Then rerun:

```sh
make
```

## RPM Installation

RPM support is planned. No RPM binary or repository is currently released.
Use the source-tarball method on RPM-based distributions with their equivalent
development packages. This does not claim those distributions were tested.

## Environment

Validation was performed on 2026-09-17 using an unprivileged process in a Debian
13 (trixie) amd64 Docker container. The host is openSUSE Tumbleweed and does not
have the required development libraries. No host packages were changed. The
prepared local Docker image is `geome-build-env:local`.

Create an equivalent isolated build environment from the repository root:

```sh
docker run --name geome-toolchain debian:trixie sh -c \
  'apt-get update && apt-get install -y --no-install-recommends build-essential pkg-config libcurl4-openssl-dev libcjson-dev clang libclang-rt-19-dev debhelper devscripts lintian groff-base man-db valgrind ca-certificates'
docker commit geome-toolchain geome-build-env:local
docker rm geome-toolchain
docker run --rm --network none --user "$(id -u):$(id -g)" \
  -v "$PWD:/work/geome" -w /work/geome geome-build-env:local \
  make check CFLAGS='-O2 -g -Werror'
```

Image setup needs network access and container root for installing dependencies.
Application builds/tests use an ordinary UID and default suites run with
`--network none`. Docker itself requires the user's normal Docker access.

## Source Build and Tests

Inside Debian with the dependencies installed:

```sh
make clean
make
make test
make check CFLAGS='-O2 -g -Werror'
make check CC=clang CFLAGS='-O2 -g -Werror'
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
  UBSAN_OPTIONS=halt_on_error=1 \
  make sanitize CC=clang CFLAGS='-O1 -g -Werror'
make clean
make check
valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=99 ./build/test_location
make install DESTDIR="$PWD/build/stage"
test "$(stat -c %a build/stage/usr/local/bin/geome)" = 755
test "$(stat -c %a build/stage/usr/local/share/man/man1/geome.1)" = 644
groff -man -Tutf8 man/geome.1 >/dev/null
make uninstall DESTDIR="$PWD/build/stage"
```

Confirmed: GCC 14 and Clang 19 strict builds, all offline unit and CLI tests,
ASan/UBSan with leak detection, staged installation modes, man-page rendering,
uninstall, and binary NOW hardening. GitHub Actions has not been run from this
local workspace; the corresponding compiler, sanitizer, package, and lintian
checks were executed locally in Debian.

Valgrind's parser/allocation-fault suite reported 1,116 allocations and 1,116
frees, zero bytes remaining, and zero errors. The debug and hardened release
targets also built successfully with warnings treated as errors.

The unsigned Debian package was built successfully with `dpkg-buildpackage`,
including the offline suite. Its contents include the executable with mode 0755
and compressed section-1 man page. Installation, unprivileged help/version/man
smoke tests, and removal passed in a fresh Debian trixie container.

The published v1.0.0 package uses the approved MIT license, GnuJason maintainer
identity, and canonical GitHub source metadata. Release lintian exited 0 with
one warning: `initial-upload-closes-no-bugs`. This does not block a GitHub release;
no Debian ITP bug or official archive inclusion has been claimed.

One explicitly enabled live request to `https://ipwho.is/` passed the JSON schema
and coordinate-range validation. TLS verification stayed enabled. No real IP or
location output was included in logs. Live requests remain excluded from default
tests, Debian builds, and CI.

The published deliverables are retained locally under `artifacts/release/final/`, including
`geome_1.0.0-1_amd64.deb`, debug symbols, buildinfo, and changes files. The source
archive is `geome-1.0.0.tar.gz`. Build and lintian logs are under
`artifacts/release/logs/`. These files are also published as assets of
[geome v1.0.0](https://github.com/GnuJason/geome/releases/tag/v1.0.0).

## Debian Commands

```sh
dpkg-buildpackage -us -uc -b
lintian ../geome_*.changes
dpkg-deb --contents ../geome_*.deb
sudo apt install ../geome_*.deb
geome --version
man geome
sudo apt remove geome
```

Run package installation/removal in a disposable Debian container when validating
from a non-Debian host. The application needs no root privileges. Debian builds
run offline tests through `dh_auto_test`; live testing is forced off.

## Demonstrate Every Mode Offline

```sh
make check
export GEOME_TEST_FIXTURE=tests/fixtures/success.json
./build/geome-fixture
./build/geome-fixture --city
./build/geome-fixture --coords
./build/geome-fixture --json
./build/geome-fixture --full
./geome --help
./geome --version
unset GEOME_TEST_FIXTURE
```

The fixture executable is test-only and never installed. Its synthetic outputs
are shown in README.md. The production binary always uses HTTPS for location.
To opt into exactly one live provider request:

```sh
GEOME_LIVE_TESTS=1 make integration-test
```

The live validator checks successful JSON structure, all nine keys, field types,
and coordinate bounds. It does not assert a city or log the location.

## Complete Source Inventory

```text
geome/
|-- LICENSE
|-- README.md
|-- CHANGELOG.md
|-- CONTRIBUTING.md
|-- Makefile
|-- .gitignore
|-- .github/workflows/ci.yml
|-- include/
|   |-- geome.h
|   |-- http_client.h
|   |-- location.h
|   |-- output.h
|   `-- exit_codes.h
|-- src/
|   |-- main.c
|   |-- cli.c
|   |-- http_client.c
|   |-- location.c
|   `-- output.c
|-- tests/
|   |-- compiler_probe.c
|   |-- fault_alloc.c
|   |-- fault_alloc.h
|   |-- fixture_http.c
|   |-- test_support.h
|   |-- test_location.c
|   |-- test_output.c
|   |-- test_http.c
|   |-- validate_json.c
|   |-- fixtures/
|   |   |-- success.json
|   |   |-- success_zero_coords.json
|   |   |-- missing_city.json
|   |   |-- missing_optional_fields.json
|   |   |-- api_failure.json
|   |   |-- malformed.json
|   |   `-- oversized.json
|   `-- integration/test_cli.sh
|-- docs/
|   |-- architecture.md
|   |-- privacy.md
|   |-- packaging.md
|   `-- validation.md
|-- man/geome.1
`-- debian/
    |-- changelog
    |-- control
    |-- copyright
    |-- rules
    |-- geome.install
    |-- geome.docs
    |-- source/format
    `-- tests/control
```

All listed files are delivered in the workspace. Generated objects, executables,
packages, logs, staging directories, and source archives are excluded from the
source inventory. The application architecture is CLI -> HTTPS -> parser -> owned
Location -> formatter; details and ownership rules are in architecture.md.

geome is released under the MIT License, copyright 2026 GnuJason. Provider terms
remain external operational requirements; see packaging.md.
