# Debian Packaging

## Identity and Release Gates

The source package, binary package, upstream project, executable, and man page
are all `geome`. The package installs `/usr/bin/geome` and
`/usr/share/man/man1/geome.1.gz`. It declares no `Conflicts`, `Replaces`, or
`Provides` against unrelated packages. Before submitting to an archive, check
source/binary package and executable namespace availability; this repository
makes no claim that the name is reserved or accepted by Debian or Ubuntu.

`PROGRAM` in the Makefile isolates the executable-name build decision. Debian
rules select `geome` explicitly; changing distribution identity requires reviewing
install manifests, man-page text, and metadata as a packaging decision.

The following owner decisions block a **public release**, not local validation:

1. Approve an actual project license and confirm copyright ownership/year.
   LICENSE currently grants no public redistribution permission. The curl and MIT
   dependency licenses permit linking with notice obligations but do not license
   this project. Review transitive dependencies for any bundled distribution.
2. Replace the explicitly local-build identity `Local Build
   <geome@localhost.localdomain>` in control and changelog with the responsible
   maintainer's real, consented name and reachable email. It is not a fabricated
   person and cannot be used for an archive upload.
3. Supply the authoritative public source repository, upstream contact, Homepage,
   and Vcs fields after those locations exist. None are invented here.
4. Review current https://ipwhois.io/terms, plan restrictions, provider privacy,
   rate limits, and intended deployment/distribution model. The free endpoint
   and permissions must not be assumed to remain unchanged.
5. Review namespace availability and replace `UNRELEASED` with an appropriate
   distribution only when a reviewed, signed release is ready.

The source uses `3.0 (quilt)`, debhelper compatibility 13, architecture `any`,
generated shlibs/misc dependencies, and a CA certificate runtime dependency.
No maintainer scripts or runtime privilege escalation are used. Debhelper runs
the full offline `make check` suite through `dh_auto_test`. Live tests are forced
off in Debian rules. `DEB_BUILD_OPTIONS=nocheck` retains standard Debian semantics
but should not be used for release validation. `Rules-Requires-Root: no` permits
an ordinary-user package build.

## Build and Inspect

```sh
sudo apt install build-essential pkg-config libcurl4-openssl-dev libcjson-dev \
  debhelper devscripts lintian ca-certificates
dpkg-buildpackage -us -uc -b
lintian ../geome_*.changes
dpkg-deb --contents ../geome_*.deb
sudo apt install ../geome_*.deb
geome --version
man geome
sudo apt remove geome
```

Use a clean Debian/Ubuntu container, sbuild, or pbuilder for independent builds.
See validation.md for the container procedure and observed lintian findings.
An unsigned binary-only local build needs no orig tarball. To prepare a source
build after the release gates are resolved:

```sh
make dist
cp geome-1.0.0.tar.gz ../geome_1.0.0.orig.tar.gz
dpkg-buildpackage -us -uc -S
```

The upstream version is declared by `VERSION` in Makefile. A release updates
that definition plus release history and Debian changelog metadata deliberately.
The man page avoids a duplicate hard-coded executable version definition.

## Distribution Is a Separate Step

A `.deb` file is a local package artifact, not an APT repository registration.
Personal APT hosting requires archive generation, signing keys and a signed
Release file, durable hosting, and user configuration scoped with `Signed-By`.
Only do that after licensing and metadata review. Debian/Ubuntu official archive
inclusion additionally needs policy compliance, accepted licensing, maintainer
review or sponsorship, and archive acceptance. None of those steps occurs merely
by running `dpkg-buildpackage`. Do not advertise public `apt install geome`
availability without naming the configured repository that actually carries it.

The CI artifact upload is disabled until the owner explicitly sets the repository
variable `GEOME_DISTRIBUTION_APPROVED` to `true` after resolving redistribution
rights. Local build artifacts remain available for validation. CI tests and lintian
still run without that variable.

## Observed Lintian Findings

Debian trixie lintian 2.122.0 reported these findings on the local binary build:

- `bogus-mail-host`: the explicit local-only address appears as Maintainer in
   the binary/debug packages and Maintainer/Changed-By in the changes file (four
   errors). The owner must supply a real address before release.
- `copyright-without-copyright-notice`: ownership has not been confirmed; a
   copyright identity must not be invented (one warning).
- `initial-upload-closes-no-bugs`: this local build does not close a Debian
   Intent To Package bug. An archive submission should file and reference a real
   ITP; a private local package need not invent one (one warning).

No lintian overrides are installed in the package. CI displays and preserves the
unfiltered report, then, for unapproved local builds only, repeats lintian with
exactly these three documented tags suppressed. Any other lintian error still
fails CI. With distribution approval enabled, the unfiltered lintian exit status
is mandatory. Warnings must be reviewed before public release. Absence of a
lintian error is not a license grant or an archive acceptance decision.
