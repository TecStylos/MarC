# MarCembly Language

## Datatypes [dt]:
- i8
- i16
- i32
- i64
- u8
- u16
- u32
- u64
- f32
- f64
***

## Registers [addr]:
Identifier | Definition
-----------|-----------
$cp | Code Pointer
$sp | Stack Pointer
$fp | Frame Pointer
$lc | Loop Counter
$ac | Accumulator
$td | Temporary Data
$ec | Exit Code
***

## Operators:
Usage | Definition
-----------|-----------
@[addr] | Dereference address
^[dt] | Size of datatype
~[i64] | Address relative to current frame pointer
***

## Instructions:
OpCode | Datatype | Arguments | Definition
-------|----------|---------- | ----------
mov | Required | [dest] : [val] | Copy [val] to [dest]
add | Required | [dest] : [val] | Add [val] to value at [dest]
sub | Required | [dest] : [val] | Subtract [val] from value at [dest]
mul | Required | [dest] : [val] | Multiply value at [dest] with [val]
div | Required | [dest] : [val] | Divide value at [dest] by [val]
drf | None | [dest] : [addr] | Dereference [addr] and store it at [dest]
conv | Required | [addr] : [dt] | Convert value at [addr] from [ocdt] datatype to [dt]
push | Required | - | Push uninitialized memory of size ^[ocdt] onto the dynamic stack
pop | Required | - | Pop value of size ^[ocdt] from the dynamic stack
pushc | Required | [val] | Push [val] onto the dynamic stack
popc | Required | [addr] | Pop value from the dynamic stack and store it at [addr]
pushf | None | - | Push a new frame onto the dynamic stack
popf | None | - | Pop the current frame from the dynamic stack
jmp | None | [addr] | Jump to [addr]
jeq | Required | [addr] : [val1] : [val2] | Jump to [addr] if val1 == val2
jne | Required | [addr] : [val1] : [val2] | Jump to [addr] if val1 != val2
jlt | Required | [addr] : [val1] : [val2] | Jump to [addr] if val1 < val2
jgt | Required | [addr] : [val1] : [val2] | Jump to [addr] if val1 > val2
jle | Required | [addr] : [val1] : [val2] | Jump to [addr] if val1 <= val2
jge | Required | [addr] : [val1] : [val2] | Jump to [addr] if val1 >= val2
alloc | Not implemented yet
free | Not implemented yet
calx | Optional | [funxAddr] : [retAddr] : [args] | Call an external function
call | Optional | [funcAddr] : [retAddr] : [args] | Call an internal function
return | None | - | Return from the current function call
exit | None | - | Stop the execution with the exit code stored at $ec (dt = i64)
***

## Directives:
Name | Datatype | Arguments | Definition
-----|----------|-----------|-----------
label | None | [name] | Store the current code address in [name].
alias | None |  [name] : [literal] | Give a literal an alias (No stack allocation).
static | None | [name] : [u64] | Reserve n bytes on the static stack and store the address in [name].
reqmod | None | [string] | Request/Require a module.
scope | None | [name] | Begin a new scope.
end | None | - | End a scope.
func | Optional | [retName] : [args] | Define an internal function.
funx | Optional | - | Tell the linker there's an external function with the specified name in an extension having the same name as the module.