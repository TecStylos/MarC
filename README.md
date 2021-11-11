# <a name="MarC"></a> MarC

## <a name="MarCLang"></a> MarC Language
MarC is going to be an interpreted C-like language.
Currently there is only the [MarCembly](#MarCemblyLang) language (MarC Assembly) available.
For more info check the [MarC Documentation](/docs/MarC.md).
***
## <a name="MarCemblyLang"></a> MarCembly Language

MarCembly is the underlying implementation of the MarC language.
For more info check the [MarCembly Documentation](/docs/MarCembly.md).
***

## <a name="MarCmdCLI"></a> MarCmd CLI
MarCmd is the command line tool for compiling/interpreting/debugging MarC and MarCembly code.
For more info check the [MarCmd Documentation](/docs/MarCmd.md)
***
## Getting Started
### Prerequisites
 * CMake
 * Make
 * C++17 Compiler

### Downloading the repo
Download the repo with `git clone --recursive https://github.com/TecStylos/MarC.git`

### Building
 1. Create a directory `/bin/<configuration>`
 2. Go into the created directory and run `cmake ../.. -DCMAKE_BUILD_TYPE=<configuration>`
 3. Run `make`

Configurations:
 * `Debug`
 * `RelWithDebInfo`
 * `Release`

## Examples
Examples for the MarCembly language can be found in the [/examples](/examples) folder.
All examples requiring the standard library should be run from the root directory. Otherwise (without specifying a [module directory](/docs/MarCmd.md#ExtensionDirectory)) the interpreter won't find the appropriate module file.

### Here the classic 'Hello world' example in MarCembly:
```MarCembly
#reqmod : "std"
calx : std>>prints : addr."Hello world!"
```