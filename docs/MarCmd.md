# MarCmd CLI

## Options
### General
 * --help
   - Show the MarCmd help.
 * --grantall
   - Grant all permissions without asking.
### Modes
 * --livecode
   - Run the MarC live interpreter.
 * --liveasm
   - Run the MarCembly live interpreter.
 * --build
   - Compile/Assemble/Link a `*.mcc/*.mca` file and store the binary in a `*.mce` file.
 * --disasm
   - Disassemble a `*.mce` file and store the MarCembly code of each module in a separate `*.mcd` file in either a specified folder or a folder with the name of the input file.
 * --debug
   - Debug a `*.mcc/*.mca/*.mce` file.
 * --interpret
   - Interpret a `*.mcc/*.mca/*.mce` file.
### I/O
 * -o _outputFile_
   - Specify the name of the output file (Ignored without `build` switch)
 * -m _moduleDirectory_
   - Specify a directory to search modules in (Can be used multiple times)
 * -e _extensionDirectory_
   - Specify a directory to search extensions in (Can be used multiple times)
### Exit behavior (Default: Keeps the interpreter open when exitCode is zero.)
 * --keeponexit
   - Keep the interpreter open after the application returned.
 * --closeonexit
   - Close the interpreter after the application returned.
 * --noexitinfo
   - Don't view the "Module '...' exited with code x." message.
### Debugging
 * --profile
   - Profile the interpreter (func/funx/instruction call counts/timings, execution time, ...)
 * --dbginfo
   - With `build` switch: Generate debug information for the application.
 * --forcerefresh
   - Force-refresh the debug window. May impact performance of the debugger and/or the application to debug.
 * --verbose
   - Show more details when building/running code.
### Miscellaneous
 * _file_
   - Any unknown argument gets interpreted as the input file.
***

## MarCembly Live Interpreter
### Multiline input
The MarCembly Live Interpreter accepts multiline input. Enter a blank line to toggle the input mode.
***

## Debugger
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
