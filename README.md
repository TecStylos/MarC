# MarC

MarC is going to be an language inspired by C.
Currently there is only the MarCembly language (MarC Assembly) available.

## MarCembly

MarCembly code can get live interpreted or compiled down to bytecode.

Feature Set
---

### Datatypes [dt]:
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

### Registers [addr]:
Identifier | Definition
-----------|-----------
$cp | Code Pointer
$sp | Stack Pointer
$fp | Frame Pointer
$lc | Loop Counter
$ac | Accumulator
$td | Temporary Data
$ec | Exit Code

### Operators:
Usage | Definition
-----------|-----------
@[addr] | Dereference address
^[dt] | Size of datatype (not implemented yet)
~[int] | Address relative to current frame pointer

### Instructions:
OpCode | Datatype Required | Arguments | Definition
-------|-------------------|---------- | ----------
mov | true | [dest] : [val] | Copy [val] to [dest]
add | true | [dest] : [val] | Add [val] to value at [dest]
sub | true | [dest] : [val] | Subtract [val] from value at [dest]
mul | true | [dest] : [val] | Multiply value at [dest] with [val]
div | true | [dest] : [val] | Divide value at [dest] by [val]
drf | false | [dest] : [addr] | Dereference [addr] and store it at [dest]
conv | true | [addr] : [dt] | Convert value at [addr] from [ocdt] datatype to [dt]
push | true | none | Push uninitialized memory of size ^[ocdt] onto the dynamic stack
pop | true | none | Pop value of size ^[ocdt] from the dynamic stack
pushc | true | [val] | Push [val] onto the dynamic stack
popc | true | [addr] | Pop value from the dynamic stack and store it at [addr]
pushf | false | none | Push a new frame onto the dynamic stack
popf | false | none | Pop the current frame from the dynamic stack
jmp | false | [addr] | Jump to [addr]
jeq | true | [addr] : [val1] : [val2] | Jump to [addr] if val1 == val2
jne | true | [addr] : [val1] : [val2] | Jump to [addr] if val1 != val2
jlt | true | [addr] : [val1] : [val2] | Jump to [addr] if val1 < val2
jgt | true | [addr] : [val1] : [val2] | Jump to [addr] if val1 > val2
jle | true | [addr] : [val1] : [val2] | Jump to [addr] if val1 <= val2
jge | true | [addr] : [val1] : [val2] | Jump to [addr] if val1 >= val2
call | true | [addr1] : [addr2] : [args] | Call [addr1] and store the return value at [addr2]
return | false | none | Return from the current function call
exit | false | none | Stop the execution with the exit code stored at $ec (dt = i64)