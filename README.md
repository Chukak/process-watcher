# process-watcher

## Build from sources

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
