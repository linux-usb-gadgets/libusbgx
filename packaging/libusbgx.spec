Name:           libusbgx
Version:        0.1.0
Release:        0
License:        LGPL-2.1+ and GPL-2.0+
Summary:        USB gadget with ConfigFS Library
Group:          Base/Device Management

Source0:        libusbgx-%{version}.tar.gz
Source1001:     libusbgx.manifest
BuildRequires:  pkg-config
BuildRequires:  pkgconfig(libconfig)

%description
libusbgx is a librarary for all USB gadget operations using ConfigFS.

%package devel
Summary:    USB gadget with ConfigFS Library
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
Development package for libusbgx. Contains headers and binaries required for
compilation of applications which use libusbgx.

%package examples
Summary:    Examples of libusbgx usage
Group:      Applications/Other
Requires:   %{name} = %{version}-%{release}

%description examples
Sample applications which shows how to use libusbgx.

%prep
%setup -q
cp %{SOURCE1001} .
%reconfigure

%build
make

%install
%make_install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%manifest %{name}.manifest
%defattr(-,root,root)
%license COPYING.LGPL
%{_libdir}/libusbgx.so.*
%{_libdir}/libusbgx.so.*.*.*

%files devel
%manifest %{name}.manifest
%defattr(-,root,root)
%{_includedir}/usbg/usbg.h
%{_includedir}/usbg/function/ffs.h
%{_includedir}/usbg/function/loopback.h
%{_includedir}/usbg/function/midi.h
%{_includedir}/usbg/function/ms.h
%{_includedir}/usbg/function/net.h
%{_includedir}/usbg/function/phonet.h
%{_includedir}/usbg/function/serial.h
%{_includedir}/usbg/function/hid.h
%{_libdir}/pkgconfig/libusbgx.pc
%{_libdir}/libusbgx.so

%files examples
%manifest %{name}.manifest
%license COPYING
%{_bindir}/gadget-acm-ecm
%{_bindir}/show-gadgets
%{_bindir}/gadget-vid-pid-remove
%{_bindir}/gadget-ffs
%{_bindir}/gadget-midi
%{_bindir}/gadget-ms
%{_bindir}/gadget-hid
%{_bindir}/gadget-export
%{_bindir}/gadget-import
%{_bindir}/show-udcs
%{_bindir}/gadget-rndis-os-desc

%changelog
