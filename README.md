# geome

`geome` answers: **Where does this machine's public IP appear to be located?**
It is a Linux-first C11 command-line client of the external HTTPS service
https://ipwho.is/. It does not determine GPS, street-level, or physical location.

**Identity:** upstream project, executable, repository, Debian binary package,
and man page are all named **geome**. The source repository and homepage are
https://github.com/GnuJason/geome. No relationship to another distribution
package is declared. This project is not in Debian or Ubuntu's official repositories.

**License:** geome is released under the [MIT License](LICENSE), copyright 2026
GnuJason. Provider terms remain a separate operational responsibility; see
[packaging](docs/packaging.md).

## Accuracy and Privacy

Results are approximate public IP-based location data. VPNs, proxies, corporate
gateways, Tor, mobile routing, and ISP address registration can show another city
or region. Never use this data for emergency response, safety decisions, legal
residency, authentication, or precise tracking.

Running a location command sends an HTTPS request from your public IP to
ipwho.is, which necessarily receives that IP. No Wi-Fi, MAC, interface data,
user-created content, telemetry, or explicit IP parameter is sent. The program
does not cache, log, retain, or forward results; it writes your selected output
to stdout. Help and version never request the network. See
[privacy details](docs/privacy.md), including proxy and OS-level caveats.

## Recommended Installation Methods

1. **Debian package (.deb):** use the GitHub Release package on a compatible
   amd64 Debian/Ubuntu system.
2. **APT repository (future):** no project APT repository is currently provided.
3. **Source tarball build:** an official, first-class installation method,
   recommended for Debian 12/LMDE and systems incompatible with the release `.deb`.
4. **RPM package (future):** planned; no RPM release package is currently provided.

The `.deb` and source tarball are available from
[GitHub Releases](https://github.com/GnuJason/geome/releases/tag/v1.0.0).
You do not need Git or a repository clone for either installation method.

## Supported Platforms

| Platform | Installation method and verification |
| --- | --- |
| Debian 12 | Source-tarball installation verified by the project owner on a Debian 12/LMDE system |
| Linux Mint Debian Edition (LMDE) | Source build verified by the project owner on that same system |
| Debian 13 (Trixie), amd64 | Package build, offline tests, and package installation verified |
| Ubuntu 24.04, amd64 | Release `.deb` installation, help/version, and live JSON schema verified |
| Other Ubuntu-family distributions | `.deb` installation when architecture and dependencies match; not individually tested |
| General Linux distributions | Source build with a C11 compiler, GNU Make, pkg-config, libcurl, and cJSON; not all distributions tested |

The v1.0.0 `.deb` depends on `libcurl4t64`. Debian 12 uses `libcurl4`, so use
the source tarball there rather than forcing package installation. Ubuntu-family
branding alone does not establish binary compatibility; the Ubuntu base and
available library versions matter. Zorin OS has not been independently tested.

## Debian Package Installation (.deb)

Download the amd64 package from the release page, or use `wget`:

```sh
wget https://github.com/GnuJason/geome/releases/download/v1.0.0/geome_1.0.0-1_amd64.deb
sudo apt update
sudo apt install ./geome_1.0.0-1_amd64.deb
geome --version
```

APT installs the required runtime dependencies. If it reports an unavailable
dependency such as `libcurl4t64`, use the source method below; do not force the
installation or replace system libraries manually. `wget` is only a download
tool; a browser download works too. SHA-256 checksums are in the release notes.

## Build From Source Tarball

This is an **official installation method**, verified by the project owner on
Debian 12 / Linux Mint Debian Edition. It builds against your system's libraries
and does not require the release `.deb` or its `libcurl4t64` package dependency.

Install build dependencies on Debian / Ubuntu:

```sh
sudo apt update
sudo apt install build-essential pkg-config \
  libcurl4-openssl-dev libcjson-dev
```

Then download, extract, build, install, and verify:

```sh
wget https://github.com/GnuJason/geome/releases/download/v1.0.0/geome-1.0.0.tar.gz
tar -xvf geome-1.0.0.tar.gz
cd geome-1.0.0
make
sudo make install
geome --version
```

Build as your ordinary user; only the system-wide install needs elevated
privileges. The default destinations are `/usr/local/bin/geome` and
`/usr/local/share/man/man1/geome.1`. Ensure `/usr/local/bin` is on your PATH.
From the extracted source directory, `sudo make uninstall` removes this install.
For an unprivileged installation, see [Source Install and Uninstall](#source-install-and-uninstall).

Other Linux distributions need equivalent development packages for a C11
compiler, GNU Make, pkg-config, libcurl, and cJSON, plus a trusted CA store at
runtime. cJSON 1.7.13 or newer is required. If `libcjson-dev` is unavailable,
install cJSON separately using your distribution's equivalent package or the
[upstream cJSON build instructions](https://github.com/DaveGamble/cJSON#building).
Its headers, library, and `libcjson.pc` must be discoverable by the compiler,
runtime linker, and pkg-config; a custom prefix may require `PKG_CONFIG_PATH`
and system linker configuration. No cJSON source is bundled with geome.

## RPM Installation

RPM support is planned. No RPM package or RPM repository is currently supplied.
On RPM-based Linux distributions, use the source tarball with the distribution's
equivalent development packages until an official RPM is released.

## Examples

The following is **synthetic documentation data**, not an assertion about your
location. Every success output ends with exactly one newline.

```text
$ geome
Charlotte, NC, US
$ geome --city
Charlotte
$ geome --coords
35.227100,-80.843100
$ geome --json
{"city":"Charlotte","region":"North Carolina","region_code":"NC","country":"United States","country_code":"US","ip":"198.51.100.10","isp":"Example ISP","lat":35.2271,"lon":-80.8431}
$ geome --full
City: Charlotte
Region: North Carolina
Region code: NC
Country: United States
Country code: US
Latitude: 35.227100
Longitude: -80.843100
IP: 198.51.100.10
ISP: Example ISP
$ geome --version
geome 1.0.0
$ geome --help
geome - approximate public IP-based location
```

Help continues with every option, privacy and accuracy notes, and exit statuses.
Short options are `-c`, `-j`, `-f`, `-h`, and `-V`. Coordinates deliberately have
no short option to avoid adding an unnecessary alias. Output modes are mutually
exclusive; repeating the same mode is harmless. Unknown flags and positional
arguments fail even alongside help. Help takes precedence over version.

Default output prefers region and country codes, falls back to full names, and
omits missing components and separators. City mode requires a city; coordinate
mode requires both coordinates. Full mode prints `Unknown` for missing values.
JSON always has nine keys: `city`, `region`, `region_code`, `country`,
`country_code`, `lat`, `lon`, `ip`, `isp`. Missing values are `null`, and present
coordinates are numbers. JSON key order is not guaranteed. Zero is a valid
coordinate, not a missing-value marker. All modes require at least one meaningful
city, region, or country identifier. Provider strings containing terminal control
characters, invalid UTF-8, or embedded NUL characters are rejected.

## Developer Build

For a repository checkout, install these additional packaging and validation
tools on Debian / Ubuntu. They are not all needed for a source-tarball install:

```sh
sudo apt update
sudo apt install build-essential pkg-config libcurl4-openssl-dev libcjson-dev \
  debhelper devscripts lintian clang groff-base man-db ca-certificates
make
./geome --help
./geome --version
```

No privilege is required to build or run. `sudo` above is only for system package
installation. cJSON 1.7.13 or newer is required for length-aware parsing. Flags
come from `pkg-config --cflags --libs libcurl libcjson`. Runtime dependencies are
libcurl, cJSON, the C library, and a trusted CA store; no curl executable, jq,
shell, or Python is invoked by the application.

Build variants and overrides:

```sh
make release
make debug
make clean
make CC=clang CFLAGS='-O2 -g -Werror'
make PROGRAM=geome VERSION=1.0.0 PREFIX=/usr/local
make dist SOURCE_DATE_EPOCH=1789646400
```

`VERSION` is the sole authoritative executable version definition. `PROGRAM`
controls the executable name and install destination without editing C source.
Renaming for publication also requires reviewing man-page text and packaging
manifests. Header dependencies and compiler/configuration changes trigger
rebuilds. Release adds optimization without dropping supplied flags. Supported
stack protector, PIE, RELRO, and NOW flags are probed; `HARDEN=0` disables the
normal Linux hardening path. Debug and sanitizer builds disable fortification.

## Tests and Validation

```sh
make test
make integration-test
make check CFLAGS='-O2 -g -Werror'
make check CC=clang CFLAGS='-O2 -g -Werror'
make sanitize CC=clang CFLAGS='-O1 -g -Werror'
make clean
make check
valgrind --leak-check=full --show-leak-kinds=all ./build/test_location
```

For Clang sanitizer builds, also install the distribution's Clang runtime
development package (`libclang-rt-dev`, or `libclang-rt-19-dev` on Debian 13).

Unit and default integration tests are fully offline. Test-only fixture transport
and linker fault injection never enter the installed executable. The oversized
fixture is a compact seed repeated past 1 MiB in the HTTP test. Sanitizers run
the whole offline suite and fail with a clear message if unsupported. Install
`valgrind` separately to use the optional command.

Live testing is **opt-in**, makes only one provider request, validates the JSON
schema and coordinate bounds without asserting a city, and does not log location:

```sh
GEOME_LIVE_TESTS=1 make integration-test
```

`GEOME_LIVE_TESTS` is a test-harness variable, not a runtime application setting.
See [validation instructions and results](docs/validation.md) for clean-container
commands, install checks, complete file inventory, and remaining release gates.

## Source Install and Uninstall

Staged installation, without changing the host:

```sh
make install DESTDIR="$PWD/build/stage"
test -x build/stage/usr/local/bin/geome
test -r build/stage/usr/local/share/man/man1/geome.1
make uninstall DESTDIR="$PWD/build/stage"
```

User-local installation:

```sh
make install PREFIX="$HOME/.local"
"$HOME/.local/bin/geome" --version
man -l "$HOME/.local/share/man/man1/geome.1"
make uninstall PREFIX="$HOME/.local"
```

Install honors `DESTDIR`, `PREFIX`, `BINDIR`, and `MANDIR`; the Makefile never
invokes sudo. Put `$HOME/.local/bin` in your PATH when using user-local installs.

## Building a Debian Package

```sh
dpkg-buildpackage -us -uc -b
lintian ../geome_*.changes
dpkg-deb --contents ../geome_*.deb
sudo apt install ../geome_*.deb
geome --version
man geome
sudo apt remove geome
```

Build a package as an ordinary user. Offline tests run through debhelper.
Building a `.deb` only creates a local artifact. Hosting it in a personal APT
repository additionally requires a signed archive, hosting, repository metadata,
and users explicitly configuring that archive. Official Debian or Ubuntu
inclusion requires licensing and packaging review and archive acceptance; neither
a local build nor a personal repository grants that status. Do not assume
`apt install geome` works without a configured repository carrying this project.

## Exit Codes and Troubleshooting

### Missing cJSON Headers

If compilation reports:

```text
fatal error: cjson/cJSON.h: No such file or directory
```

Install the cJSON development package (`sudo apt install libcjson-dev` on
Debian / Ubuntu). If unavailable, build and install cJSON from source as
described in [Build From Source Tarball](#build-from-source-tarball), then rerun:

```sh
make
```

The runtime library alone does not provide development headers.

### Exit Codes

| Status | Meaning | Action |
| --- | --- | --- |
| 0 | Success | Consume stdout |
| 2 | Invalid usage | Run `geome --help`; select one mode |
| 3 | Network, DNS, TLS, timeout, truncated transfer | Check connectivity, CA store, clock, and proxy settings |
| 4 | HTTP or service failure | For 429, try later; otherwise check provider status and terms |
| 5 | Invalid or incomplete location | Try default/JSON for missing city or coordinates; report sanitized parser failures |
| 6 | Local memory/internal error | Check available memory and library installation |
| 7 | Output/write failure | Check the destination, disk space, or closed pipe |

Diagnostics start with `geome:` on stderr. Data/service failures emit no normal
output. Write failures can leave partial output and cannot retract bytes already
written. SIGPIPE is ignored so closed pipes yield status 7 rather than a crash.
Never work around TLS failures by disabling verification. libcurl honors proxy
environment variables described in the man page; the apparent location can be
the proxy's egress location. The application does not load curl CLI config files.

## Limits and Third Parties

One HTTPS request, no redirects, no retries, 5-second connect timeout, 10-second
total timeout, and a 1 MiB decompressed-response cap. There is no caching,
configuration, provider fallback, explicit-IP lookup, or precise positioning.
Provider failures and database inaccuracies are outside this client's control.

Transport uses [libcurl](https://curl.se/), parsing and serialization use
[cJSON](https://github.com/DaveGamble/cJSON), and location data comes from
[IPWHOIS.io](https://ipwhois.io/). These dependencies are dynamically linked;
no provider documentation or database is bundled.
Provider [terms](https://ipwhois.io/terms), [privacy policy](https://ipwhois.io/privacy),
plan restrictions, fair-use rules, and rate limits can change. No free-plan
availability, commercial-use permission, or precise retry time is promised.
