![build-test](https://github.com/Chukak/process-watcher/actions/workflows/cmake.yml/badge.svg)

# process-watcher

Shows some information about the observed process.

![process-watcher](https://github.com/Chukak/process-watcher/blob/main/process-watcher.png)



## Installation and Running

### Debian/Ubuntu

To install the `process-watcher` from the `.deb` package, download the release you need from [Releases](https://github.com/Chukak/process-watcher/releases).
Any `.deb` package has the format `process-watcher_<VERSION>_<OS-VERSIONS>_<ARCHITECRUTE>.deb`. After downloading run the following command:
```bash
apt install ./<pkgname>.deb
# or for Ubuntu 14.04 and Debian Jessie
dpkg -i ./<pkgname>.deb
```

To run the `process-watcher`:
```bash
process-watcher # show help
# Example:
process-watcher kwin_x11 # watching for kwin_x11
```


### Windows 10

Download the `process-watcher_1.0.0_windows-10_installer.exe` from [Releases](https://github.com/Chukak/process-watcher/releases). 
This is a self-extracting archive, created by `7-Zip`. Run this downloaded file in the folder you need.

To run the `process-watcher` open `cmd.exe` (or you can to use `powershell`):
```bash
cd /d /path/to/process-watcher.exe # for cmd.exe
process-watcher.exe # show help
# Example:
process-watcher.exe notepad++ # watching for notepad++
```


## Building from sources

### Linux

To build the `process-watcher` from sources on Linux you will need `libncurses-dev`:
```bash
apt-get install libpthread-stubs0-dev libncurses-dev # or libncurses5-dev
```

Open terminal and run the following command:
```bash
cmake .
make
sudo make install # for installing
```

Tested on `Kubuntu 20.04/18.04/14.04`, `Debian Buster/Stretch/Jessie`.


### Windows

To build the `process-watcher` from sources on Windows you will need [PDcurses](https://github.com/wmcbrine/PDCurses) for Windows or from other sources. 
Also, you will need `cmake` and `mingw` compiler. Compile the `.dll` and `.a` libraries before compiling the `process-watcher`. 
Open the terminal and run the folliwing command:
```bash
cmake -G "MinGW Makefiles" -DPDCURSES_USE_DLL=1 -DPDCURSES_INCLUDE_PATH="/path/to/include" -DPDCURSES_LIB_PATH="/path/to/lib" .
mingw32-make
```

Tested on Windows: `Windows 10`.
