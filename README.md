# process-watcher

Shows some information about the observed process.

![process-watcher](https://github.com/Chukak/process-watcher/blob/main/process-watcher.png)

## Build from sources

### Linux

To build the `process-watcher` from sources on Linux you will need `libncurses-dev`:
```bash
apt-get install libncurses-dev
```

Open terminal and run the following command:
```bash
cmake .
make
sudo make install # for installing
```

To run the `process-watcher`:
```bash
./process-watcher # show help
# Example:
./process-watcher kwin_x11 # watching for kwin_x11
```

Tested on `Kubuntu 20.04`.


### Windows

To build the `process-watcher` from sources on Windows you will need [PDcurses](https://github.com/wmcbrine/PDCurses) for Windows or from other sources. 
Also, you will need `cmake` and `mingw` compiler. Compile the `.dll` and `.a` libraries before compiling the `process-watcher`. 
Open the terminal and run the folliwing command:
```bash
cmake -G "MinGW Makefiles" -DPDCURSES_USE_DLL=1 -DPDCURSES_INCLUDE_PATH="/path/to/include" -DPDCURSES_LIB_PATH="/path/to/lib" .
mingw32-make
```

To run the `process-watcher`:
```bash
process-watcher.exe # show help
# Example:
process-watcher.exe notepad++ # watching for notepad++
```

Tested on Windows: `Windows 10`.
