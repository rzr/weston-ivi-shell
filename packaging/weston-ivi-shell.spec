%bcond_with x
%bcond_with wayland

Name:           weston-ivi-shell
Version:        1.4.0
Release:        0
Summary:        Wayland Compositor Infrastructure
License:        MIT
Group:          Graphics & UI Framework/Wayland Window System
Url:            http://weston.freedesktop.org/
#Git-Clone:	git://anongit.freedesktop.org/wayland/weston
#Git-Web:	http://cgit.freedesktop.org/wayland/weston/

Source0:         %name-%version.tar.xz
BuildRequires:	autoconf >= 2.64, automake >= 1.11
BuildRequires:  pkgconfig
BuildRequires:  expat-devel
BuildRequires:  pkgconfig(egl) >= 7.10
BuildRequires:  mesa-devel
BuildRequires:  wayland-devel
BuildRequires:  pkgconfig(glesv2)
BuildRequires:  mesa-libEGL-devel
BuildRequires:  libwayland-egl
BuildRequires:  pkgconfig(cairo)
BuildRequires:  pkgconfig(cairo-egl) >= 1.11.3
BuildRequires:  pkgconfig(mtdev) >= 1.1.0
BuildRequires:  pkgconfig(pixman-1)
BuildRequires:  libjpeg-devel
BuildRequires:  libtool >= 2.2
BuildRequires:  libvpx-devel
BuildRequires:  pam-devel
BuildRequires:  pkgconfig(mtdev) >= 1.1.0
BuildRequires:  pkgconfig(pixman-1)
BuildRequires:  pkgconfig(poppler-glib)
BuildRequires:  pkgconfig(wayland-client)
BuildRequires:  pkgconfig(wayland-egl)
BuildRequires:  pkgconfig(wayland-server)
BuildRequires:  pkgconfig(xkbcommon) >= 0.3.0
%if %{with x}
BuildRequires:  libGLU-devel
BuildRequires:  libXcursor-devel
%endif

Requires:       weston-startup
Requires(pre):  /usr/sbin/groupadd

%if !%{with wayland}
ExclusiveArch:
%endif


%description
Weston is the reference implementation of a Wayland compositor, and a
useful compositor in its own right. Weston has various backends that
lets it run on Linux kernel modesetting and evdev input as well as
under X11. Weston ships with a few example clients, from simple
clients that demonstrate certain aspects of the protocol to more
complete clients and a simplistic toolkit. There is also a quite
capable terminal emulator (weston-terminal) and an toy/example
desktop shell. Finally, weston also provides integration with the
Xorg server and can pull X clients into the Wayland desktop and act
as a X window manager.


%package devel
Summary: Development files for package %{name}
Group:   Graphics & UI Framework/Development
%description devel
This package provides header files and other developer releated files
for package %{name}.

%package clients
Summary: Sample clients for package %{name}
Group:   Graphics & UI Framework/Development
%description clients
This package provides a set of example wayland clients useful for
validating the functionality of wayland with very little dependencies
on other system components.

%prep
%setup -q

%autogen \
    --disable-static \
    --disable-setuid-install \
    --enable-simple-clients \
    --enable-clients \
    --disable-libunwind \
    --disable-xwayland \
    --disable-xwayland-test \
    --disable-x11-compositor \
    --disable-rpi-compositor \
    %{?extra_config_options:%extra_config_options} \
    #eol

%build
#make %{?jobs:-j%jobs} V=1
make -j1 V=1 

%install
%make_install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%pre
getent group weston-launch >/dev/null || %{_sbindir}/groupadd -o -r weston-launch

%docs_package

%files
%defattr(-,root,root)
%license COPYING
%{_bindir}/*
%{_libdir}/weston/*.so
%{_datadir}/*
%{_libexecdir}/*


%files devel
%defattr(-,root,root,-)
%{_includedir}/*
%{_libdir}/pkgconfig/*
