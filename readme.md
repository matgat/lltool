# [lltool](https://github.com/matgat/lltool.git)
[![linux-build](https://github.com/matgat/lltool/actions/workflows/linux-build.yml/badge.svg)](https://github.com/matgat/lltool/actions/workflows/linux-build.yml)
[![ms-build](https://github.com/matgat/lltool/actions/workflows/ms-build.yml/badge.svg)](https://github.com/matgat/lltool/actions/workflows/ms-build.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

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
> [!NOTE]
> Windows binary is dynamically linked to Microsoft c++ runtime,
> so needs the installation of
> [`VC_redist.x64.exe`](https://aka.ms/vs/17/release/vc_redist.x64.exe)
> as prerequisite.


### Command line invocation:

The program invocation in the most generic form:

```bat
$ lltool [task] [arguments in any order]
```

In a little more detail:

```bat
$ lltool [update|convert|help] [switches] [path(s)]
```

The first argument selects the task; the order of the subsequent arguments
is not relevant, except when the command switch must be followed by a value
such as `--out/--to` or `--options`.

> [!TIP]
> * The switches can be passed in a brief notation using a single hyphen,
>   for example `-vF` is equivalent to `--verbose --force`.
> * The program won't overwrite an existing file unless you `--force` it.

To check all the available command line arguments:

```bat
$ lltool help
```

### Exit values

| Return value | Meaning                                |
|--------------|----------------------------------------|
|      0       | Operation successful                   |
|      1       | Operation completed but with issues    |
|      2       | Operation aborted due to a fatal error |


### Examples

To update a project:

```bat
$ lltool update "C:\path\to\project.ppjs"
```
> [!CAUTION]
> The choice to backup or not the original file is yours: do it in your script.

To update a project without overwriting the original file:

```bat
$ lltool update "C:\path\to\project.ppjs" --to "C:\path\to\project-updated.ppjs" --force
```

> [!TIP]
> Use `--force` to overwrite the output file if existing


To convert all `.h` files in `prog/` and all `.pll` files in `plc/`,
deleting existing files in the given output folder and
indicating some conversion options:

```bat
$ lltool convert --options no-timestamp,sort,plclib-indent:2 --force --to plc/LogicLab/generated-libs  prog/*.h plc/*.pll
```

> [!TIP]
> * `.h` files will generate both `.pll` and `.plclib` files, while `.pll` files a `.plclib`
> * An error will be raised in case of library name clashes
> * Use `--force` to overwrite and clear the directory content


To reconvert a `.pll` file to a given output file:

```bat
$ lltool convert file.pll --force --to path/to/output.pll
```
> [!IMPORTANT]
> File extension does matter to choose the desired parsing and writing functions



_________________________________________________________________________
## Updating a project
The operation consists in parsing a project file detecting the contained
libraries linked to an external file (`<lib name="...">`) and updating
their content in the project file.
If the external libraries are stored in a different encoding,
their content will be embedded respecting the original project file
encoding.
> [!NOTE]
> This operation has a strong guarantee: in case of ordinary runtime errors
> the filesystem is left in the same state as before the invocation.


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

> [!IMPORTANT]
> File type is determined by its extension (must be lowercase)


### Limitations
The following limitations are introduced to maximize efficiency:

* The program assumes well formed projects
* Line breaks won't be converted: if the external libraries and
  the project file use a different end of line character sequence,
  the resulting file will contain mixed line breaks
* plclib content won't be reindented (see `plclib-indent`)



_________________________________________________________________________
## Converting to library
The operation consists in taking some input files and translating their
content in another format.
It is possible to indicate multiple input files using a glob
pattern, in that case an output directory must be specified;
in case of duplicate file base names the program will exit with error.

> [!NOTE]
> This operation offers no guarantee in case of ordinary runtime
> errors: an output file could be left incomplete and others may
> have been already written, leaving the set of libraries in a
> incoherent state.

The supported conversions are:

* `.h` → `.pll`, `.plclib`
* `.pll` → `.plclib`

> [!TIP]
> These will be the default conversions when the output file is not explicitly specified

Where

* `.h` (Sipro header)
* `.pll` (Plc LogicLab3 Library)
* `.plclib` (LogicLab5 PLC LIBrary)

> [!WARNING]
> File type is determined by its extension, mind the case!

Sipro header files resemble a *c-like* header containing just
`#define` directives.
LogicLab libraries are *text* files embedding *IEC 61131-3* ST code;
the latest format (`.plclib`) embraces a pervasive *xml* structure.
More about these formats below.


### Conversion options
It is possible to specify a set of comma separated `key:value` pairs
to provide some control on the produced output.
The recognized keys are:

|   key              |    value            |               description               |
|--------------------|---------------------|-----------------------------------------|
| `no-timestamp`     |                     | Don't put a timestamp in generated file |
| `sort`             |                     | Sort PLC elements and variables by name |
| `plclib-schemaver` | *\<uint\>.\<uint\>* | Schema version of generated plclib file |
| `plclib-indent`    | *\<uint\>*          | Tabs indentation of `<lib>` content     |

Example:

```bat
$ lltool convert --options plclib-schemaver:2.8,plclib-indent:1,no-timestamp,sort  ...
```


### Limitations
The following limitations are introduced to maximize efficiency:

* Input files must be encoded in `utf-8`
* Input files must be syntactically correct
* Input files should use preferably unix line breaks (`\n`)
* In windows, paths containing uppercase non ASCII characters may
  compromise name collision checks *(no lowercase converter for unicode)*
* Descriptions (`.h` `#define` inlined comments and `.pll` `{DE: ...}`)
  cannot contain XML special characters nor line breaks
* Check syntax limitations below


_________________________________________________________________________
### Syntax of Sipro header files
Sipro `.h` files supported syntax is:

```
// line comment
/*  -------------
    block comment
    ------------- */
#define reg_label register // description
#define val_label value // [type] description
```

Inlined comments have meaning and define the resource description.

Sipro registers will be recognized and exported:

```cpp
#define vqPos vq100 // Position
```

It's possible to export also numeric constants
declaring their *IEC 61131-3* type as in:

```cpp
#define PI 3.14159 // [LREAL] Circumference/diameter ratio
```

The supported types are:

| type        | description                 | size | range                     |
| ----------- | --------------------------- | ---- | ------------------------- |
|   `BOOL`    | *BOOLean*                   |  1   | FALSE\|TRUE               |
|   `SINT`    | *Short INTeger*             |  1   | -128 … 127                |
|   `INT`     | *INTeger*                   |  2   | -32768 … 32767            |
|   `DINT`    | *Double INTeger*            |  4   |  -2147483648 … 2147483647 |
| ~~`LINT`~~  | ~~*Long INTeger*~~          |  8   |~~-2<sup>63</sup> … 2<sup>63</sup>-1~~|
|   `USINT`   | *Unsigned Short INTeger*    |  1   | 0 … 255                   |
|   `UINT`    | *Unsigned INTeger*          |  2   | 0 … 65535                 |
|   `UDINT`   | *Unsigned Double INTeger*   |  4   | 0 … 4294967295            |
| ~~`ULINT`~~ | ~~*Unsigned Long INTeger*~~ |  8   |~~0 … 2<sup>64</sup>-1~~   |
| ~~`REAL`~~  | ~~*REAL number*~~           |  4   |~~±10<sup>38</sup>~~       |
|   `LREAL`   | *Long REAL number*          |  8   | ±10<sup>308</sup>         |
|   `BYTE`    | *1 byte*                    |  1   | 0 … 255                   |
|   `WORD`    | *2 bytes*                   |  2   | 0 … 65535                 |
|   `DWORD`   | *4 bytes*                   |  4   | 0 … 4294967295            |
| ~~`LWORD`~~ | ~~*8 bytes*~~               |  8   |~~0 … 2<sup>64</sup>-1~~   |


_________________________________________________________________________
### Syntax of `.pll` files

Refer to *IEC 61131-3* `ST` syntax.
The parser has the following limitations:

* Is case sensitive: recognizes only uppercase keywords (`PROGRAM`, ...)
* Tested only on pure Structured Text projects
* Not supported:
  * Multidimensional arrays like `ARRAY[1..2, 1..2]`
  * `RETAIN` specifier
  * Pointers
  * LogicLab extension for conditional compilation `{IFDEF}`
  * Latest standard (`INTERFACE`, `THIS`, `PUBLIC`, `IMPLEMENTS`, `METHOD`, …)

Authors are encouraged to embed custom additional library data in
the first comment of the `.pll` file.
The recognized fields are `descr` and `version`, for example:

```
(*
    author: ignored
    descr: Machine logic
    version: 1.2.31
*)
```


_________________________________________________________________________
## Author's biased opinions
Some limitations of this program come directly from the
biased opinions of the author about the following topics:

* Text file encoding: `utf-8` it's the only sane choice
  * <sub>Rationale: `8-bit` encodings and codepages must
    be finally dropped because incomplete, ambiguous and
    not portable.
	`utf-32` is a huge waste of space and cache
	(at least with files that are mostly `ASCII`),
	with the disadvantage of having to deal with endianness
	and to be less compatible with old tools.
	I'm not considering at all `utf-16`, the worst possible
	choice because combines all the drawbacks of the others
	with a very little gain</sub>
* Line breaks should be uniformed to `\n` (`LF`)
  * <sub>Rationale: no technical reason for two-chars
    lines breaks nowadays, this just introduces avoidable
    annoyances working on different platforms and a useless
    waste of space and time</sub>
* Mind the case of file names
  * <sub>Rationale: most filesystems are case sensitive,
    so better take into account that</sub>



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

> [!TIP]
> If building a version that needs `{fmt}`,
> install the dependency beforehand with
> your package manager:
> 
> ```sh
> $ sudo pacman -S fmt
> ```
> 
> or
> 
> ```sh
> $ sudo apt install -y libfmt-dev
> ```


### Windows

On Windows you need at least Microsoft Visual Studio 2022 (Community Edition).
Once you have `msbuild` visible in path, you can launch the build from the command line:

```bat
> msbuild build/lltool.vcxproj -t:Rebuild -p:Configuration=Release -p:Platform=x64
```

> [!TIP]
> If building a version that needs `{fmt}`
> install the dependency beforehand with `vcpkg`:
> 
> ```bat
> > git clone https://github.com/Microsoft/vcpkg.git
> > cd .\vcpkg
> > .\bootstrap-vcpkg.bat -disableMetrics
> > .\vcpkg integrate install
> > .\vcpkg install fmt:x64-windows
> ```
>
> To just update the `vcpkg` libraries:
>
> ```bat
> > cd .\vcpkg
> > git pull
> > .\bootstrap-vcpkg.bat -disableMetrics
> > .\vcpkg upgrade --no-dry-run
> ```
