Name:         stsdb
Version:      1.0
Release:      alt1

Summary:      Simple time series database.
Group:        System
URL:          https://github.com/slazav/stsdb
License:      GPL

Packager:     Vladislav Zavjalov <slazav@altlinux.org>

Source:       %name-%version.tar
BuildRequires: libmicrohttpd-devel libjansson-devel
Requires:      libmicrohttpd libjansson

%description
STSDB -- a simple time series database

%prep
%setup -q

%build
%makeinstall

%files
%dir %_sharedstatedir/stsdb
%_bindir/stsdb
%_bindir/stsdb_http

%changelog