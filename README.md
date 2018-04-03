# kernel-based keylogger for Linux.
A simplex kernel-based keylogger written for fun, not evil.


### Functionality
The keylogger can do the following:
- Hide itself from loadable kernel modules list
- Protect against being unloaded by the user
- Unhide itself

### Supported Platforms
The keylogger was tested to work on Linux kernels 4.8.0-52 and 4.10 TLS as provided by Ubuntu in Ubuntu 16.04 LTS and Ubuntu 16.10 respectively, but it should be very easy to port to kernels in-between, as well as newer ones.

### Setting Up Environment
Install a compiler, Linux headers and all other things required for us to build the keylogger:
```
apt-get update
apt-get install build-essential
```

### Build
> make

### Use
To install the keylogger module:
> sudo insmod AKeylogger.ko

Test whether the module is loaded:
> lsmod | grep "AKeylogger"
> dmesg

Test whether the logging is happening:
> cat /proc/AKeylog
The log file will show the keystrokes logged after the module has been loaded.

To uninstall the keylogger module:
> sudo rmmod AKeylogger


License
This project is licensed under [GPLv3](LICENSE).
