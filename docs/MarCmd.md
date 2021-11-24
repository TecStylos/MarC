# MarCmd CLI

## Options
### General
 * --help
 * --grantall
### Modes
 * --livecode
 * --liveasm
 * --build
 * --interpret
### I/O
 * -o _outputFile_
 * -m _moduleDirectory_
 * -e _extensionDirectory_
### Exit behavior
 * --keeponexit
 * --closeonexit
### Debugging
 * --profile
 * --debug
 * --verbose
### Miscellaneous
 * [file]
***

## Examples
### Run the live MarCembly assembler and close the CLI after the interpreter has exited:
```
MarCmd --liveasm --closeonexit
```
### Execute a code file with all needed permissions and keep the console open after the module has exited:
```
MarCmd --verbose --keeponexit --grantall examples/GameOfLife.mca
```