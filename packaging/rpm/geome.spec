Name:           geome
Version:        1.0.0
Release:        1
Summary:        Lightweight CLI tool for geolocation and network metadata
License:        MIT
URL:            https://github.com/GnuJason/geome
Source0:        geome-1.0.0.tar.gz

BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  pkgconfig
BuildRequires:  pkgconfig(libcurl)
BuildRequires:  pkgconfig(libcjson)
Requires:       ca-certificates

%description
Geome is a lightweight, fast CLI tool for retrieving user geolocation and
network metadata, including city, region, country, coordinates, IP address,
and ISP.

%prep
%setup -q

%build
%make_build CFLAGS="%{optflags}"

%install
%make_install CFLAGS="%{optflags}" PREFIX=%{_prefix} BINDIR=%{_bindir} MANDIR=%{_mandir}

%check
make check CFLAGS="%{optflags}"

%files
%license LICENSE
%doc README.md CHANGELOG.md
%{_bindir}/geome
%{_mandir}/man1/geome.1%{?ext_man}

%changelog
* Sat Sep 19 2026 Jason <gnujason@mailfence.com> - 1.0.0-1
- Initial RPM release for openSUSE