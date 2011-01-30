#
# Spec file for creating VirtualBox rpm packages
#

#
# Copyright (C) 2006-2010 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License as published by the Free Software Foundation,
# in version 2 as it comes in the "COPYING" file of the VirtualBox OSE
# distribution. VirtualBox OSE is distributed in the hope that it will
# be useful, but WITHOUT ANY WARRANTY of any kind.
#

%define %SPEC% 1
%{!?python_sitelib: %define python_sitelib %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib()")}

Summary:   Oracle VM VirtualBox
Name:      %NAME%
Version:   %BUILDVER%_%BUILDREL%
Release:   1
URL:       http://www.virtualbox.org/
Source:    VirtualBox.tar.bz2
License:   GPLv2
Group:     Applications/System
Vendor:    Oracle Corporation
BuildRoot: %BUILDROOT%
Requires:  %LIBASOUND%

%if %{?rpm_suse:1}%{!?rpm_suse:0}
%debug_package
%endif

%MACROSPYTHON%


%description
VirtualBox is a powerful PC virtualization solution allowing
you to run a wide range of PC operating systems on your Linux
system. This includes Windows, Linux, FreeBSD, DOS, OpenBSD
and others. VirtualBox comes with a broad feature set and
excellent performance, making it the premier virtualization
software solution on the market.


%prep
%setup -q
DESTDIR=""
unset DESTDIR


%build


%install
# Mandriva: prevent replacing 'echo' by 'gprintf'
export DONT_GPRINTIFY=1
rm -rf $RPM_BUILD_ROOT
install -m 755 -d $RPM_BUILD_ROOT/sbin
install -m 755 -d $RPM_BUILD_ROOT%{_initrddir}
install -m 755 -d $RPM_BUILD_ROOT/lib/modules
install -m 755 -d $RPM_BUILD_ROOT/etc/vbox
install -m 755 -d $RPM_BUILD_ROOT/usr/bin
install -m 755 -d $RPM_BUILD_ROOT/usr/src
install -m 755 -d $RPM_BUILD_ROOT/usr/share/applications
install -m 755 -d $RPM_BUILD_ROOT/usr/share/pixmaps
install -m 755 -d $RPM_BUILD_ROOT/usr/share/icons/hicolor
install -m 755 -d $RPM_BUILD_ROOT%{_defaultdocdir}/virtualbox
install -m 755 -d $RPM_BUILD_ROOT/usr/lib/virtualbox
install -m 755 -d $RPM_BUILD_ROOT/usr/lib/virtualbox/ExtensionPacks
install -m 755 -d $RPM_BUILD_ROOT/usr/share/virtualbox
install -m 755 -d $RPM_BUILD_ROOT/usr/share/mime/packages
mv VBoxEFI32.fd $RPM_BUILD_ROOT/usr/lib/virtualbox || true
mv VBoxEFI64.fd $RPM_BUILD_ROOT/usr/lib/virtualbox || true
mv *.gc $RPM_BUILD_ROOT/usr/lib/virtualbox
mv *.r0 $RPM_BUILD_ROOT/usr/lib/virtualbox
mv *.rel $RPM_BUILD_ROOT/usr/lib/virtualbox || true
mv VBoxNetDHCP $RPM_BUILD_ROOT/usr/lib/virtualbox
mv VBoxNetAdpCtl $RPM_BUILD_ROOT/usr/lib/virtualbox
mv VBoxXPCOMIPCD $RPM_BUILD_ROOT/usr/lib/virtualbox
mv components $RPM_BUILD_ROOT/usr/lib/virtualbox/components
mv *.so $RPM_BUILD_ROOT/usr/lib/virtualbox
mv *.so.4 $RPM_BUILD_ROOT/usr/lib/virtualbox || true
mv VBoxTestOGL $RPM_BUILD_ROOT/usr/lib/virtualbox
mv vboxshell.py $RPM_BUILD_ROOT/usr/lib/virtualbox
(export VBOX_INSTALL_PATH=/usr/lib/virtualbox && \
  cd ./sdk/installer && \
  %{__python} ./vboxapisetup.py install --prefix %{_prefix} --root $RPM_BUILD_ROOT)
rm -rf sdk/installer
mv sdk $RPM_BUILD_ROOT/usr/lib/virtualbox
mv nls $RPM_BUILD_ROOT/usr/share/virtualbox
cp -a src $RPM_BUILD_ROOT/usr/share/virtualbox
mv VBox.sh $RPM_BUILD_ROOT/usr/bin/VBox
mv VBoxSysInfo.sh $RPM_BUILD_ROOT/usr/share/virtualbox
mv VBoxCreateUSBNode.sh $RPM_BUILD_ROOT/usr/share/virtualbox
cd icons
  for i in *; do
    install -d $RPM_BUILD_ROOT/usr/share/icons/hicolor/$i/mimetypes
    mv $i/* $RPM_BUILD_ROOT/usr/share/icons/hicolor/$i/mimetypes
    rmdir $i
  done
cd -
rmdir icons
mv virtualbox.xml $RPM_BUILD_ROOT/usr/share/mime/packages
for i in VBoxManage VBoxSVC VBoxSDL VirtualBox VBoxHeadless VBoxExtPackHelperApp vboxwebsrv webtest; do
  mv $i $RPM_BUILD_ROOT/usr/lib/virtualbox; done
for i in VBoxSDL VirtualBox VBoxHeadless VBoxNetDHCP VBoxNetAdpCtl; do
  chmod 4511 $RPM_BUILD_ROOT/usr/lib/virtualbox/$i; done
mv VBoxTunctl $RPM_BUILD_ROOT/usr/bin
for d in /lib/modules/*; do
  if [ -L $d/build ]; then
    rm -f /tmp/vboxdrv-Module.symvers
    ./src/vboxhost/vboxdrv/build_in_tmp \
      --save-module-symvers /tmp/vboxdrv-Module.symvers \
      KBUILD_VERBOSE= KERN_DIR=$d/build MODULE_DIR=$RPM_BUILD_ROOT/$d/misc -j4 \
      %INSTMOD%
    ./src/vboxhost/vboxnetflt/build_in_tmp \
      --use-module-symvers /tmp/vboxdrv-Module.symvers \
      KBUILD_VERBOSE= KERN_DIR=$d/build MODULE_DIR=$RPM_BUILD_ROOT/$d/misc -j4 \
      %INSTMOD%
    ./src/vboxhost/vboxnetadp/build_in_tmp \
      --use-module-symvers /tmp/vboxdrv-Module.symvers \
      KBUILD_VERBOSE= KERN_DIR=$d/build MODULE_DIR=$RPM_BUILD_ROOT/$d/misc -j4 \
      %INSTMOD%
  fi
done
mv kchmviewer $RPM_BUILD_ROOT/usr/lib/virtualbox
for i in rdesktop-vrdp.tar.gz rdesktop-vrdp-keymaps additions/VBoxGuestAdditions.iso; do
  mv $i $RPM_BUILD_ROOT/usr/share/virtualbox; done
if [ -d accessible ]; then
  mv accessible $RPM_BUILD_ROOT/usr/lib/virtualbox
fi
mv rdesktop-vrdp $RPM_BUILD_ROOT/usr/bin
install -D -m 755 vboxdrv.init $RPM_BUILD_ROOT%{_initrddir}/vboxdrv
%if %{?rpm_suse:1}%{!?rpm_suse:0}
ln -sf ../etc/init.d/vboxdrv $RPM_BUILD_ROOT/sbin/rcvboxdrv
%endif
install -D -m 755 vboxweb-service.init $RPM_BUILD_ROOT%{_initrddir}/vboxweb-service
%if %{?rpm_suse:1}%{!?rpm_suse:0}
ln -sf ../etc/init.d/vboxweb-service $RPM_BUILD_ROOT/sbin/rcvboxweb-service
%endif
ln -s VBox $RPM_BUILD_ROOT/usr/bin/VirtualBox
ln -s VBox $RPM_BUILD_ROOT/usr/bin/virtualbox
ln -s VBox $RPM_BUILD_ROOT/usr/bin/VBoxManage
ln -s VBox $RPM_BUILD_ROOT/usr/bin/vboxmanage
ln -s VBox $RPM_BUILD_ROOT/usr/bin/VBoxSDL
ln -s VBox $RPM_BUILD_ROOT/usr/bin/vboxsdl
ln -s VBox $RPM_BUILD_ROOT/usr/bin/VBoxVRDP
ln -s VBox $RPM_BUILD_ROOT/usr/bin/VBoxHeadless
ln -s VBox $RPM_BUILD_ROOT/usr/bin/vboxheadless
ln -s VBox $RPM_BUILD_ROOT/usr/bin/vboxwebsrv
ln -s /usr/share/virtualbox/src/vboxhost $RPM_BUILD_ROOT/usr/src/vboxhost-%VER%
mv virtualbox.desktop $RPM_BUILD_ROOT/usr/share/applications/virtualbox.desktop
mv VBox.png $RPM_BUILD_ROOT/usr/share/pixmaps/VBox.png


%pre
# defaults
[ -r /etc/default/virtualbox ] && . /etc/default/virtualbox

# check for active VMs of the installed (old) package
VBOXSVC_PID=`pidof VBoxSVC 2>/dev/null || true`
if [ -n "$VBOXSVC_PID" ]; then
  # executed before the new package is installed!
  if [ -f /etc/init.d/vboxweb-service ]; then
    # try graceful termination; terminate the webservice first
    /etc/init.d/vboxweb-service stop 2>/dev/null || true
    # ask the daemon to terminate immediately
    kill -USR1 $VBOXSVC_PID
  fi
  sleep 1
  if pidof VBoxSVC > /dev/null 2>&1; then
    echo "A copy of VirtualBox is currently running.  Please close it and try again."
    echo "Please note that it can take up to ten seconds for VirtualBox (in particular"
    echo "the VBoxSVC daemon) to finish running."
    exit 1
  fi
fi

# check for old installation
if [ -r /etc/vbox/vbox.cfg ]; then
  . /etc/vbox/vbox.cfg
  if [ "x$INSTALL_DIR" != "x" -a -d "$INSTALL_DIR" ]; then
    echo "An old installation of VirtualBox was found. To install this package the"
    echo "old package has to be removed first. Have a look at /etc/vbox/vbox.cfg to"
    echo "determine the installation directory of the previous installation. After"
    echo "uninstalling the old package remove the file /etc/vbox/vbox.cfg."
    exit 1
  fi
fi

# XXX remove old modules from previous versions (disable with INSTALL_NO_VBOXDRV=1 in /etc/default/virtualbox)
if [ "$INSTALL_NO_VBOXDRV" != "1" ]; then
  find /lib/modules -name "vboxdrv\.*" 2>/dev/null|xargs rm -f 2> /dev/null || true
  find /lib/modules -name "vboxnetflt\.*" 2>/dev/null|xargs rm -f 2> /dev/null || true
  find /lib/modules -name "vboxnetadp\.*" 2>/dev/null|xargs rm -f 2> /dev/null || true
fi


%post
LOG="/var/log/vbox-install.log"

# defaults
[ -r /etc/default/virtualbox ] && . /etc/default/virtualbox

# remove old cruft
if [ -f /etc/init.d/vboxdrv.sh ]; then
  echo "Found old version of /etc/init.d/vboxdrv.sh, removing."
  rm /etc/init.d/vboxdrv.sh
fi
if [ -f /etc/vbox/vbox.cfg ]; then
  echo "Found old version of /etc/vbox/vbox.cfg, removing."
  rm /etc/vbox/vbox.cfg
fi
rm -f /etc/vbox/module_not_compiled

# install udev rule (disable with INSTALL_NO_UDEV=1 in /etc/default/virtualbox)
if [ -d /etc/udev/rules.d -a "$INSTALL_NO_UDEV" != "1" ]; then
  udev_call=""
  udev_app=`which udevadm 2> /dev/null`
  if [ $? -eq 0 ]; then
    udev_call="${udev_app} version 2> /dev/null"
  else
    udev_app=`which udevinfo 2> /dev/null`
    if [ $? -eq 0 ]; then
      udev_call="${udev_app} -V 2> /dev/null"
    fi
  fi
  udev_fix="="
  if [ "${udev_call}" != "" ]; then
    udev_out=`${udev_call}`
    udev_ver=`expr "$udev_out" : '[^0-9]*\([0-9]*\)'`
    if [ "$udev_ver" = "" -o "$udev_ver" -lt 55 ]; then
      udev_fix=""
    fi
  fi
  usb_createnode="/usr/share/virtualbox/VBoxCreateUSBNode.sh"
  echo "KERNEL=${udev_fix}\"vboxdrv\", NAME=\"vboxdrv\", OWNER=\"root\", GROUP=\"root\", MODE=\"0600\"" \
    > /etc/udev/rules.d/10-vboxdrv.rules
  echo "SUBSYSTEM=${udev_fix}\"usb_device\", ACTION=${udev_fix}\"add\", RUN=\"${usb_createnode} \$major \$minor \$attr{bDeviceClass}\"" \
    >> /etc/udev/rules.d/10-vboxdrv.rules
  echo "SUBSYSTEM=${udev_fix}\"usb\", ACTION=${udev_fix}\"add\", ENV{DEVTYPE}==\"usb_device\", RUN=\"${usb_createnode} \$major \$minor \$attr{bDeviceClass}\"" \
    >> /etc/udev/rules.d/10-vboxdrv.rules
  echo "SUBSYSTEM=${udev_fix}\"usb_device\", ACTION=${udev_fix}\"remove\", RUN=\"${usb_createnode} --remove \$major \$minor\"" \
    >> /etc/udev/rules.d/10-vboxdrv.rules
  echo "SUBSYSTEM=${udev_fix}\"usb\", ACTION=${udev_fix}\"remove\", ENV{DEVTYPE}==\"usb_device\", RUN=\"${usb_createnode} --remove \$major \$minor\"" \
    >> /etc/udev/rules.d/10-vboxdrv.rules
fi
# Remove old udev description file
if [ -f /etc/udev/rules.d/60-vboxdrv.rules ]; then
  rm -f /etc/udev/rules.d/60-vboxdrv.rules 2> /dev/null
fi
# Build our device tree
for i in /sys/bus/usb/devices/*; do
  if test -r "$i/dev"; then
    dev="`cat "$i/dev" 2> /dev/null`"
    major="`expr "$dev" : '\(.*\):' 2> /dev/null`"
    minor="`expr "$dev" : '.*:\(.*\)' 2> /dev/null`"
    class="`cat $i/bDeviceClass 2> /dev/null`"
    sh ${usb_createnode} "$major" "$minor" "$class" ${usb_group} 2>/dev/null
  fi
done

# XXX SELinux: allow text relocation entries
%if %{?rpm_redhat:1}%{!?rpm_redhat:0}
if [ -x /usr/bin/chcon ]; then
  chcon -t texrel_shlib_t /usr/lib/virtualbox/*VBox* > /dev/null 2>&1
  chcon -t texrel_shlib_t /usr/lib/virtualbox/VirtualBox.so > /dev/null 2>&1
  chcon -t texrel_shlib_t /usr/lib/virtualbox/VBoxAuth.so > /dev/null 2>&1
  chcon -t texrel_shlib_t /usr/lib/virtualbox/components/VBox*.so > /dev/null 2>&1
  chcon -t java_exec_t    /usr/lib/virtualbox/VirtualBox > /dev/null 2>&1
  chcon -t java_exec_t    /usr/lib/virtualbox/VBoxSDL > /dev/null 2>&1
  chcon -t java_exec_t    /usr/lib/virtualbox/VBoxHeadless > /dev/null 2>&1
  chcon -t java_exec_t    /usr/lib/virtualbox/VBoxExtPackHelperApp > /dev/null 2>&1
  chcon -t java_exec_t    /usr/lib/virtualbox/vboxwebsrv > /dev/null 2>&1
fi
%endif

# create users groups (disable with INSTALL_NO_GROUP=1 in /etc/default/virtualbox)
if [ "$INSTALL_NO_GROUP" != "1" ]; then
  echo
  echo "Creating group 'vboxusers'. VM users must be member of that group!"
  echo
  groupadd -f vboxusers 2> /dev/null
fi
%if %{?rpm_redhat:1}%{!?rpm_redhat:0}
/sbin/chkconfig --add vboxdrv
/sbin/chkconfig --add vboxweb-service
%endif
%if %{?rpm_suse:1}%{!?rpm_suse:0}
%{fillup_and_insserv -f -y -Y vboxdrv vboxweb-service}
%endif
%if %{?rpm_mdv:1}%{!?rpm_mdv:0}
/sbin/ldconfig
%_post_service vboxdrv
%_post_service vboxweb-service
%update_menus
%endif
update-mime-database /usr/share/mime &> /dev/null || :
update-desktop-database -q > /dev/null 2>&1 || :
touch --no-create /usr/share/icons/hicolor
gtk-update-icon-cache -q /usr/share/icons/hicolor 2> /dev/null || :

# Disable module compilation with INSTALL_NO_VBOXDRV=1 in /etc/default/virtualbox
BUILD_MODULES=0
REGISTER_MODULES=1
if [ ! -f /lib/modules/`uname -r`/misc/vboxdrv.ko ]; then
  REGISTER_MODULES=0
  if [ "$INSTALL_NO_VBOXDRV" != "1" ]; then
    # compile problem
    cat << EOF
No precompiled module for this kernel found -- trying to build one. Messages
emitted during module compilation will be logged to $LOG.

EOF
    BUILD_MODULES=1
  fi
fi
# if INSTALL_NO_VBOXDRV is set to 1, remove all shipped modules
if [ "$INSTALL_NO_VBOXDRV" = "1" ]; then
  rm -f /lib/modules/*/misc/vboxdrv.ko
  rm -f /lib/modules/*/misc/vboxnetflt.ko
  rm -f /lib/modules/*/misc/vboxnetadp.ko
fi
if [ $BUILD_MODULES -eq 1 ]; then
  /etc/init.d/vboxdrv setup || true
else
  if lsmod | grep -q "vboxdrv[^_-]"; then
    /etc/init.d/vboxdrv stop || true
  fi
  if [ $REGISTER_MODULES -eq 1 ]; then
    DKMS=`which dkms 2>/dev/null`
    if [ -n "$DKMS" ]; then
      $DKMS remove -m vboxhost -v %VER% --all > /dev/null 2>&1 || true
    fi
  fi
  /etc/init.d/vboxdrv start > /dev/null
fi
/etc/init.d/vboxweb-service start > /dev/null


%preun
# $1==0: remove the last version of the package
# $1==1: install the first time
# $1>=2: upgrade
%if %{?rpm_suse:1}%{!?rpm_suse:0}
%stop_on_removal vboxweb-service
%endif
%if %{?rpm_mdv:1}%{!?rpm_mdv:0}
%_preun_service vboxweb-service
%endif
%if %{?rpm_redhat:1}%{!?rpm_redhat:0}
if [ "$1" = 0 ]; then
  /sbin/service vboxweb-service stop > /dev/null
  /sbin/chkconfig --del vboxweb-service
fi
%endif

if [ "$1" = 0 ]; then
  # check for active VMs
  VBOXSVC_PID=`pidof VBoxSVC 2>/dev/null || true`
  if [ -n "$VBOXSVC_PID" ]; then
    kill -USR1 $VBOXSVC_PID
    sleep 1
    if pidof VBoxSVC > /dev/null 2>&1; then
      echo "A copy of VirtualBox is currently running.  Please close it and try again."
      echo "Please note that it can take up to ten seconds for VirtualBox (in particular"
      echo "the VBoxSVC daemon) to finish running."
      exit 1
    fi
  fi
fi
%if %{?rpm_suse:1}%{!?rpm_suse:0}
%stop_on_removal vboxdrv
%endif
%if %{?rpm_mdv:1}%{!?rpm_mdv:0}
%_preun_service vboxdrv
%endif
if [ "$1" = 0 ]; then
%if %{?rpm_redhat:1}%{!?rpm_redhat:0}
  /sbin/service vboxdrv stop > /dev/null
  /sbin/chkconfig --del vboxdrv
%endif
  rm -f /etc/udev/rules.d/10-vboxdrv.rules
  rm -f /etc/vbox/license_agreed
  rm -f /etc/vbox/module_not_compiled
  # remove our USB device tree
  rm -rf /dev/vboxusb 2> /dev/null
fi
DKMS=`which dkms 2>/dev/null`
if [ -n "$DKMS" ]; then
  $DKMS remove -m vboxhost -v %VER% --all > /dev/null 2>&1 || true
fi


%postun
%if %{?rpm_redhat:1}%{!?rpm_redhat:0}
if [ "$1" -ge 1 ]; then
  /sbin/service vboxdrv restart > /dev/null 2>&1
  /sbin/service vboxweb-service restart > /dev/null 2>&1
fi
%endif
%if %{?rpm_suse:1}%{!?rpm_suse:0}
%restart_on_update vboxdrv vboxweb-service
%insserv_cleanup
%endif
%if %{?rpm_mdv:1}%{!?rpm_mdv:0}
/sbin/ldconfig
%{clean_desktop_database}
%clean_menus
%endif
update-mime-database /usr/share/mime &> /dev/null || :
update-desktop-database -q > /dev/null 2>&1 || :
touch --no-create /usr/share/icons/hicolor
gtk-update-icon-cache -q /usr/share/icons/hicolor 2> /dev/null || :


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root)
%doc LICENSE
%doc UserManual*.pdf
%doc VirtualBox*.chm
%{_initrddir}/vboxdrv
%{_initrddir}/vboxweb-service
%{?rpm_suse: %{py_sitedir}/*}
%{!?rpm_suse: %{python_sitelib}/*}
%{?rpm_suse: /sbin/rcvboxdrv}
%{?rpm_suse: /sbin/rcvboxweb-service}
/lib/modules
/etc/vbox
/usr/bin
/usr/src/vbox*
/usr/lib/virtualbox
/usr/share/applications
/usr/share/icons
/usr/share/mime/packages
/usr/share/pixmaps
/usr/share/virtualbox