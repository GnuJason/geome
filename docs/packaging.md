# Debian Packaging

## Identity

The source package, binary package, upstream project, executable, and man page
are all `geome`. The package installs `/usr/bin/geome` and
`/usr/share/man/man1/geome.1.gz`. It declares no `Conflicts`, `Replaces`, or
`Provides` against unrelated packages. Before submitting to an archive, check
source/binary package and executable namespace availability; this repository
makes no claim that the name is reserved or accepted by Debian or Ubuntu.

`PROGRAM` in the Makefile isolates the executable-name build decision. Debian
rules select `geome` explicitly; changing distribution identity requires reviewing
install manifests, man-page text, and metadata as a packaging decision.

The project is MIT licensed, copyright 2026 GnuJason. The Debian Maintainer and
upstream contact are GnuJason <gnujason@mailfence.com>. The authoritative source,
Homepage, Vcs-Browser, and Vcs-Git are https://github.com/GnuJason/geome.
libcurl's curl license and cJSON's MIT license permit dynamic linking; they remain
separate dependencies with their own copyright notices. No dependency source is
bundled. Review transitive dependencies when distributing a combined image.

Before submitting to a distribution archive, review current
https://ipwhois.io/terms, plan restrictions, provider privacy, rate limits, and
the intended distribution model. Provider terms, availability, and permissions
are external operational requirements that can change.

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

CI runs the full offline test and package suite, lintian, and uploads build logs
and package artifacts for pushes and pull requests. Live tests remain disabled.

## Observed Lintian Findings

The pre-release local package used temporary metadata and had the following
historical lintian findings. They are expected to be resolved by the v1.0.0
metadata below; rerun lintian after the release package is built.

- `bogus-mail-host`: the explicit local-only address appears as Maintainer in
   the binary/debug packages and Maintainer/Changed-By in the changes file (four
   errors). The owner must supply a real address before release.
- `copyright-without-copyright-notice`: ownership has not been confirmed; a
   copyright identity must not be invented (one warning).
- `initial-upload-closes-no-bugs`: this local build does not close a Debian
   Intent To Package bug. An archive submission should file and reference a real
   ITP; a private local package need not invent one (one warning).

No lintian overrides are installed. CI fails on the unfiltered lintian exit
status. An absence of lintian findings does not by itself establish archive
acceptance; Debian and Ubuntu retain their independent review processes.
