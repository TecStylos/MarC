# MarCembly Language

## Datatypes:
Datatype | C Equivalent (stdint.h)
---------|------------------------
i8 | int8_t
i16 | int16_t
i32 | int32_t
i64 | int64_t
u8 | uint8_t
u16 | uint16_t
u32 | uint32_t
u64 | uint64_t
f32 | float (32 bit)
f64 | double (64 bit)
addr | void* (Per byte arithmetics)
***

## Registers:
Identifier | Name | Explanation
-----------|------|------------
$cp | Code Pointer | Stores the address of the next instruction to execute.
$sp | Stack Pointer | Stores the address of the next free byte on the dynamic stack.
$fp | Frame Pointer | Stores the base address of the current frame (of the dynamic stack).
$lc | Loop Counter | Mostly used for iterative variables.
$ac | Accumulator | General purpose register.
$td | Temporary Data | Currently unused.
$ec | Exit Code | Holds the exit code of the application. (Returned to the interpreter on call to `exit`)
***

## Operators:
Usage | Explanation
------|------------
@[addr] | Dereference the given address (May be used 0-3 times in a row. E.g.: @@$ac)
^[dt] | Insert the size of the datatype `dt`.
~[offset] | Address relative to the current frame pointer. `offset` may be negative.
%[n] | Insert the name stored at the `n`th element from the top of the preprocessor stack.
***

## Instructions:
(Arguments with * are optional)
OpCode | Datatype | Arguments | Explanation
-------|----------|---------- | -----------
mov | Required | [dest] : [val] | Copy `val` to `dest`.
add | Required | [dest] : [val] | Add `val` to value at `dest`.
sub | Required | [dest] : [val] | Subtract `val` from value at `dest`.
mul | Required | [dest] : [val] | Multiply value at `dest` with `val`.
div | Required | [dest] : [val] | Divide value at `dest` by `val`.
inc | Required | [dest] | Increment the value stored at `dest`.
dec | Required | [dest] | Decrement the value stored at `dest`.
conv | Required | [addr] : [dt] | Convert value at `addr` from `ocdt` datatype to `dt`.
push | Required | - | Push uninitialized memory of size `^ocdt` onto the dynamic stack.
pop | Required | - | Pop value of size `^ocdt` from the dynamic stack.
pushn | None | [size] | Push `size` bytes onto the dynamic stack.
popn | None | [size] | Pop `size` bytes from the dynamic stack.
pushc | Required | [val] | Push `val` onto the dynamic stack.
popc | Required | [addr] | Pop value from the dynamic stack and store it at `addr`.
pushf | None | - | Push a new frame onto the dynamic stack.
popf | None | - | Pop the current frame from the dynamic stack.
jmp | None | [addr] | Jump to `addr`.
jeq | Required | [addr] : [val1] : [val2] | Jump to `addr` if `val1` _==_ `val2`.
jne | Required | [addr] : [val1] : [val2] | Jump to `addr` if `val1` _!=_ `val2`.
jlt | Required | [addr] : [val1] : [val2] | Jump to `addr` if `val1` _<_ `val2`.
jgt | Required | [addr] : [val1] : [val2] | Jump to `addr` if `val1` _>_ `val2`.
jle | Required | [addr] : [val1] : [val2] | Jump to `addr` if `val1` _<=_ `val2`.
jge | Required | [addr] : [val1] : [val2] | Jump to `addr` if `val1` _>=_ `val2`.
alloc | None | [addr] : [size] | Allocate `size` bytes and store the address in `addr`.
free | None | [extAddr] | Free memory allocated with alloc.
calx | Optional | [funxAddr] *[ : retAddr] *[ : typedArgs] | Call an external function.
call | Optional | [funcAddr] *[ : retAddr] *[ : typedArgs] | Call an internal function.
return | None | - | Return from the current function call.
exit | None | - | Stop the execution and return the exit code @$ec (dt == i64).
***

## Directives:
(Arguments with * are optional)
Name | Datatype | Arguments | Explanation
-----|----------|-----------|------------
label | None | [name] | Store the current code address in [name].
alias | None |  [name] : [literal] | Give a literal an alias (No stack allocation). (Literals can be other aliases, labels, names for static allocations, scopes, (extern) function names, local variables, ...)
static | None | [name] : [size] | Reserve n bytes on the static stack and create an alias with name `name` holding the address.
reqmod | None | [string] | Request/Require a module.
extension | None | [string] | Request an extension. (Required for external functions)
manperm | None | [funcName] | Request a mandatory permission for an external function.
optperm | None | [funcName] | Request an optional permission for an external function.
scope | None | [name] | Begin a new scope.
end | None | - | End a scope/macro.
func | Optional | [funcName] *[ : retName] *[ : typedArgs] | Define an internal function. Place '`!`' in front of `funcName` to mark it as a function without additional local variables.
funx | None | [funcName] | Tell the linker there's an external function with the specified name in an extension having the same name as the module.
local | None | [name] : [size] | Reserve local (frame relative) memory (frame offset automatically determined).
macro | Optional | [macroName] *[ : macroArgs] | Define a macro.
ppush | None | [name] | Push a name onto the preprocessor stack.
ppop | None | - | Pop a name from the preprocessor stack.
prep | None | [n] : [name] | Replace the `n`th element from the top of the preprocessor stack with `name`.
