Summary:        Library for generating random numbers using the RdRand instruction on Intel CPUs
Name:           RdRand
Version:        1.0.0
Release:        1%{?dist}
License:        LGPLv2+
Group:          Applications/System
URL:            http://github.com/BroukPytlik/%{name}
Source0:        https://github.com/BroukPytlik/%{name}/archive/%{version}.tar.gz
BuildRequires: autoconf libtool
ExcludeArch: armv7hl

%description
RdRand is an instruction for returning random numbers from an Intel on-chip hardware random number generator. RdRand is available in Ivy Bridge and later processors.

It uses cascade construction, combining a HW RNG operating at 3Gbps with CSPRNG into one sealed block on CPU. The entropy source is a metastable circuit, with unpredictable behavior based on thermal
noise. The entropy is fed into a 3:1 compression ratio entropy extractor (whitener) based on AES-CBC-MAC. Online statistical tests are performed at this stage and only high quality random data are used as the seed for cryptograhically secure SP800-90 AES-CTR DRBG compliant PRNG. 
This generator is producing maximum of 512 128-bit AES blocks before it's reseeded. According to documentation the 512 blocks is a upper limit for reseed, in practice it reseeds much more frequently.

%package devel
Summary:        Development files for the RdRand
Group:          Development/Libraries
Requires:       %{name} = %{version}-%{release}

%description devel
Headers and shared object symbolic links for the RdRand.


%prep
%setup -q

%build
%configure
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT INSTALL="%{__install} -p"
rm -f $RPM_BUILD_ROOT{%{_libdir}/librdrand.la,%{_libdir}/librdrand/include/rdrandconfig.h}
#rm -f $RPM_BUILD_ROOT{% {_libdir}/libaa.la,% {_infodir}/dir}

# clean up multilib conflicts
#touch -r NEWS $RPM_BUILD_ROOT% {_datadir}/rdrand-gen # $RPM_BUILD_ROOT% {_datadir}/librdrand.m4

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%doc README COPYING ChangeLog NEWS
%{_bindir}/rdrand-gen
%{_mandir}/man7/rdrand-gen.7.gz
%{_libdir}/librdrand.so.*

%files devel
%defattr(-,root,root,-)
%{_mandir}/man3/librdrand.3.gz
%{_includedir}/librdrand.h
%{_libdir}/librdrand.so
%{_libdir}/pkgconfig/*
#% {_infodir}/librdrand.info*
#% {_datadir}/aclocal.m4

%changelog
* Wed Dec 04 2013 Jan Tulak <jan@tulak.me> - 1.0.0-1
- Created.
