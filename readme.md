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



### Command line invocation

```bat
> lltool [task] [arguments in any order]
```

The first argument selects the task; the order of the subsequent arguments
is not relevant, except when the command switch must be followed by a value
such as `--out` or `--options`.
The switches can be passed in a brief notation using a single hyphen,
for example `-vF` is equivalent to `--verbose --force`.

To check the available command line switches:

```bat
> lltool --help
```


### Examples

To update a project:

```bat
> lltool update "C:\path\to\project.ppjs"
```

> ***❗The choice to backup or not the original file is yours: do it in your script❗***

To update a project without overwriting the original file:

```bat
> lltool update "C:\path\to\project.ppjs" --to "C:\path\to\project-updated.ppjs" --force
```

> ***Use `--force` to overwrite the output file if existing***

To convert a `.h` file to the corresponding `.pll` and `.plclib` in a
given directory:

```sh
$ lltool convert file.h --force --to path/to/dir
```

> ***Use `--force` to allow write in the given directory if already existing***

To convert all `.h` files in `prog/` and all `.pll` files in `plc/`,
deleting existing files in the given output folder and
indicating some conversion options:

```sh
$ lltool convert --options plclib-schemaver:2.8,plclib-indent:1 --force --to plc/LogicLab/generated-libs  prog/*.h plc/*.pll
```

> ***❗Beware of output file name clashes of globbed input files❗***



_________________________________________________________________________
## Updating a project
The operation consists in parsing a project file detecting the contained
libraries linked to an external file (`<lib name="...">`) and updating
their content in the project file.
If the external libraries are stored in a different encoding,
their content will be embedded respecting the original project file
encoding.
This operation has a strong guarantee: in case of typical runtime errors
(ex. parsing errors) the filesystem is left in the same as before the
invocation.


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

> ***File type is determined by its extension (must be lowercase)***


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
This operation doesn't offer any guarantee in case of runtime errors:
an output file could have been left in a corrupted state.

The supported conversions are:
* `.h` → `.pll`, `.plclib`
* `.pll` → `.plclib`

Where
* `.h` (Sipro header)
* `.pll` (Plc LogicLab3 Library)
* `.plclib` (LogicLab5 PLC LIBrary)

> ***File type is determined by its extension, mind the case!***

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
* Descriptions (`.h` `#define` inlined comments and `.pll` `{DE: ...}`)
  cannot contain XML special characters nor line breaks

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
## Author's biased opinions
Some limitations of this program come directly from the
biased opinions of the author about the following topics:
* Text file encoding: `UTF-8` it's the only sane choice
  _Rationale: `8-bit ANSI` must be finally dropped because
              incomplete, ambiguous and not portable.
              All the others (`UTF-16`, `UTF-32`), for
              these mostly `ASCII` sources, have only
              disadvantages: huge waste of space and
              cache, endianness, less compatible with
              old tools. `UTF-16` is the worst choice
              of all, combining the previous drawbacks
              with having a variable number of codepoints_
* Line breaks should be finally uniformed to _unix_ (`LF`, `\n`)
  _Rationale: No technical reason for two-chars `\r\n` lines breaks,
              unless supporting ancient teletype devices, this just
              introduces avoidable waste of space, (parsing) time
              and annoyances when using different operating systems_
* Mind the case of file names
  _Rationale: `Windows` users often don't pay attention if a file
              has a lowercase or uppercase name, but the rest of
              the world do, so better take into account that_



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

On Windows you need at least Microsoft Visual Studio 2022 (Community Edition).
Once you have `msbuild` visible in path, you can launch the build from the command line:

```bat
> msbuild build/lltool.vcxproj -t:Rebuild -p:Configuration=Release -p:Platform=x64
```

If building a version that needs `{fmt}`
install the dependency beforehand with `vcpkg`:

```bat
> git clone https://github.com/Microsoft/vcpkg.git
> cd .\vcpkg
> .\bootstrap-vcpkg.bat -disableMetrics
> .\vcpkg integrate install
> .\vcpkg install fmt:x64-windows
```

To just update the `vcpkg` libraries:

```bat
> cd .\vcpkg
> git pull
> .\bootstrap-vcpkg.bat -disableMetrics
> .\vcpkg upgrade --no-dry-run
```
