import apport.hookutils

def add_info(report):
    """Add a list of installed packages matching 'virtualbox' or 'linux-headers'"""
    report['VirtualBoxOse.DpkgList'] = apport.hookutils.command_output(["sh", "-c", "dpkg -l | grep -e virtualbox -e linux-headers"])
    
    """Add information about installed VirtualBox kernel modules"""
    report['VirtualBoxOse.ModInfo'] = apport.hookutils.command_output(["sh", "-c",
        "find /lib/modules/`uname -r` -name \"vbox*\" | xargs -r modinfo"])
    
    report['LsMod'] = apport.hookutils.command_output(["lsmod"])
