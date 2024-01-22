## [lltool](https://github.com/matgat/lltool.git)
[![linux-build](https://github.com/matgat/lltool/actions/workflows/linux-build.yml/badge.svg)](https://github.com/matgat/lltool/actions/workflows/linux-build.yml)
[![ms-build](https://github.com/matgat/lltool/actions/workflows/ms-build.yml/badge.svg)](https://github.com/matgat/lltool/actions/workflows/ms-build.yml)

A tool capable to manipulate LogicLab files.
The main functions are:

* Update libraries in a LogicLab project (`.ppjs`, `.plcprj`)
* Conversion to LogicLab libraries (`.pll`, `.plclib`)

One of the main goals is to perform these tasks in the most efficient way:
this introduces some limitations on the accepted input files, mainly
related to using the raw content of input files avoiding dynamic memory
allocations as much as possible.



_________________________________________________________________________
## Usage
> 📝 Windows binary is dynamically linked to Microsoft c++ runtime,
> so needs the installation of
> [`VC_redist.x64.exe`](https://aka.ms/vs/17/release/vc_redist.x64.exe)
> as prerequisite.

Command line structure:

```bat
> lltool [update|convert] [switches] [path]
```

| Return value | Meaning                                |
|--------------|----------------------------------------|
|      0       | Operation successful                   |
|      1       | Operation completed but with issues    |
|      2       | Operation aborted due to a fatal error |


### Examples
To print usage info:

```bat
> lltool --help
```

To update a project:

```bat
> lltool update "C:\path\to\project.ppjs"
```

> ***❗The choice to backup or not the original file is yours: do in your script❗***

To update a project without overwriting the original file:

```bat
> lltool update "C:\path\to\project.ppjs" --out "C:\path\to\project-updated.ppjs"
```

> ***❗The output file will be overwritten without any confirmation❗***

To convert all `.h` files in the current directory into a given output directory:

```sh
$ lltool convert --output plc/LogicLab/generated-libs  prog/*.h
```

To convert all `.h` files in `prog/` and all `.pll` files in `plc/`,
deleting existing files in the output folder,
sorting objects by name and indicating the `plclib` schema version:

```sh
$ lltool convert --fussy --options sort:by-name,schemaver:2.8 --clear --output plc/LogicLab/generated-libs  prog/*.h plc/*.pll
```


_________________________________________________________________________
## Updating a project
The operation consists in parsing a project file detecting the contained
libraries linked to an external file (`<lib name="...">`) and updating
their content in the project file.
If the external libraries are stored in a different encoding,
their content will be embedded respecting the original project file
encoding.

The supported project formats are:
|           |                     |
|-----------|---------------------|
| `.ppjs`   | (LogicLab3 project) |
| `.plcprj` | (LogicLab5 project) |

The supported library formats are:
|           |                         |
|-----------|-------------------------|
| `.pll`    | (Plc LogicLab3 Library) |
| `.plclib` | (LogicLab5 PLC LIBrary) |


### Limitations
The following limitations are introduced to maximize efficiency:
* The program assumes well formed files
* Line breaks won't be converted: if the external libraries and
the project file use a different end of line character sequence,
the resulting file will contain mixed line breaks
* plclib content won't be reindented


_________________________________________________________________________
## Converting to library
The operation consists in taking some input files and translating their
content in another format.

The supported conversions are:
* `.h` → `.pll`, `.plclib`
* `.pll` → `.plclib`

Where
* `.h` (Sipro header)
* `.pll` (Plc LogicLab3 Library)
* `.plclib` (LogicLab5 PLC LIBrary)

Sipro header files resemble a *c-like* header containing just
`#define` directives.
LogicLab libraries are *text* files embedding *IEC 61131-3* ST code;
the latest format (`.plclib`) embraces a pervasive *xml* structure.
More about these formats below.


### Limitations
The following limitations are introduced to maximize efficiency:
* Input files must be encoded in `UTF-8`
* Input files must be syntactically correct
* Input files should use preferably unix line breaks (`\n`)
* Descriptions (`.h` `#define` inlined comments and `.pll` `{DE: ...}`) cannot contain XML special characters nor line breaks

_IEC 61131-3_ syntax
* Parser is case sensitive (uppercase keywords)
* Not supported:
  * Multidimensional arrays like `ARRAY[1..2, 1..2]`
  * `RETAIN` specifier
  * Pointers
  * `IFDEF` LogicLab extension for conditional compilation
  * Latest standard (`INTERFACE`, `THIS`, `PUBLIC`, `IMPLEMENTS`, `METHOD`, …)


_________________________________________________________________________
### `.h` files
Sipro header files supported syntax is:

```c
// line comment
/*  -------------
    block comment
    ------------- */
#define reg_label register // description
#define val_label value // [type] description
```

Inlined comments have meaning and define the resource description.

Sipro registers will be recognized and exported:

```c
#define vqPos vq100 // Position
```

It's possible to export also numeric constants
declaring their *IEC 61131-3* type as in:

```c
#define PI 3.14159 // [LREAL] Circumference/diameter ratio
```

The recognized types are:

| type        | description                 | size | range                     |
| ----------- | --------------------------- | ---- | ------------------------- |
|   `BOOL`    | *BOOLean*                   |  1   | FALSE/TRUE                |
|   `SINT`    | *Short INTeger*             |  1   | -128 … 127                |
|   `INT`     | *INTeger*                   |  2   | -32768 … 32767            |
|   `DINT`    | *Double INTeger*            |  4   |  -2147483648 … 2147483647 |
| ~~`LINT`~~  | ~~*Long INTeger*~~          |  8   | -2⁶³ … 2⁶³-1              |
|   `USINT`   | *Unsigned Short INTeger*    |  1   | 0 … 255                   |
|   `UINT`    | *Unsigned INTeger*          |  2   | 0 … 65535                 |
|   `UDINT`   | *Unsigned Double INTeger*   |  4   | 0 … 4294967295            |
| ~~`ULINT`~~ | ~~*Unsigned Long INTeger*~~ |  8   | 0 … 18446744073709551615  |
| ~~`REAL`~~  | ~~*REAL number*~~           |  4   | ±10³⁸                     |
|   `LREAL`   | *Long REAL number*          |  8   | ±10³⁰⁸                    |
|   `BYTE`    | *1 byte*                    |  1   | 0 … 255                   |
|   `WORD`    | *2 bytes*                   |  2   | 0 … 65535                 |
|   `DWORD`   | *4 bytes*                   |  4   | 0 … 4294967295            |
| ~~`LWORD`~~ | ~~*8 bytes*~~               |  8   | 0 … 18446744073709551615  |


_________________________________________________________________________
### `.pll` files
Authors are encouraged to include custom additional library data in
the first comment of the `.pll` file.
The recognized fields are:

```
(*
    descr: Machine logic
    version: 1.2.31
*)
```


_________________________________________________________________________
## Text files encoding and like breaks
Some limitations of this program come directly from the author biased
opinions about the following topics:
* Text file encoding: `UTF-8` it's the only sane choice
  _Rationale: `8-bit ANSI` must be finally dropped because
              incomplete, ambiguous and not portable.
              All the others (`UTF-16`, `UTF-32`), for
              these mostly `ASCII` sources, have only
              disadvantages: huge waste of space and
              cache, endianness, less compatible with
              old tools_
* Line breaks should be finally uniformed to _unix_ (`LF`, `\n`)
  _Rationale: No technical reason for two-chars `\r\n` lines breaks,
              unless supporting ancient teletype devices, this just
              introduces avoidable waste of space, (parsing) time
              and annoyances when using different operating systems_


_________________________________________________________________________
## Build

Check the operations in the python script:

```sh
$ git clone https://github.com/matgat/lltool.git
$ cd lltool
$ python build/build.py
```

To run tests:

```sh
$ python tests/run-unit-tests.py
$ python tests/run-system-tests.py
$ python tests/run-manual-tests.py
```


### linux
With a `g++` toolchain you can invoke `make` directly:

```sh
$ cd build
$ make
```

To run tests:

```sh
$ make test
```

If building a version that needs `{fmt}`,
install the dependency beforehand with
your package manager:

```sh
$ sudo pacman -S fmt
```

or

```sh
$ sudo apt install -y libfmt-dev
```

### Windows

On Windows you need the latest Microsoft Visual Studio (Community Edition).
Once you have `msbuild` visible in path, you can launch the build from the command line:

```bat
> msbuild build/lltool.vcxproj -t:Rebuild -p:Configuration=Release -p:Platform=x64
```

If building a version that needs `{fmt}`
install the dependency beforehand with `vcpkg`:

```bat
> git clone https://github.com/Microsoft/vcpkg.git
> .\vcpkg\bootstrap-vcpkg.bat -disableMetrics
> .\vcpkg\vcpkg integrate install
> .\vcpkg\vcpkg install fmt:x64-windows
```

To just update the libraries:

```bat
> cd .\vcpkg
> git pull
> .\bootstrap-vcpkg.bat -disableMetrics
> .\vcpkg upgrade --no-dry-run
```
