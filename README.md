# MarC

## MarC Language
MarC is going to be an interpreted C-like language.
Currently there is only the [MarCembly](#MarCemblyLang) language (MarC Assembly) available.
For more info check the [MarC Documentation](./docs/MarC.md).
***
## MarCembly Language

MarCembly is the underlying implementation of the MarC language.
For more info check the [MarCembly Documentation](./docs/MarCembly.md).
***

## MarCmd CLI
MarCmd is the command line tool for compiling/interpreting/debugging MarC and MarCembly code.
For more info check the [MarCmd Documentation](./docs/MarCmd.md)
***

## Getting Started
### Prerequisites
 * CMake
 * Make
 * C++17 Compiler
 * ANSI Escape Code compatible terminal (Required for the [Debugger](./docs/MarCmd.md) and some [examples](./examples/))

### Downloading the repo
Download the repo with `git clone --recursive https://github.com/TecStylos/MarC.git`

### Building
On Unix systems run the [./build.sh](./build.sh) script in the repo's root directory.
The script prompts you for a configuration. Valid options are listed below.

Configurations:
 * `Debug`
 * `RelWithDebInfo`
 * `Release`

On Windows use Visual Studio to build the projects.
***

## Examples
Examples for the MarCembly language can be found in the [./examples/](./examples/) folder.
All examples requiring the standard library should be run from the root directory. Otherwise (without [specifying a module directory](./docs/MarCmd.md)) the interpreter won't find the appropriate module file.

### Here the classic 'Hello world' example in MarCembly:
```MarCembly
#reqmod : "std"
#manperm : >>stdext>>prints
println : "Hello world!"
```
