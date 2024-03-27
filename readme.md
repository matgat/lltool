## [lltool](https://github.com/matgat/lltool.git)
[![linux-build](https://github.com/matgat/lltool/actions/workflows/linux-build.yml/badge.svg)](https://github.com/matgat/lltool/actions/workflows/linux-build.yml)
[![ms-build](https://github.com/matgat/lltool/actions/workflows/ms-build.yml/badge.svg)](https://github.com/matgat/lltool/actions/workflows/ms-build.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

A tool capable to manipulate *LogicLab* files.
The main functions are:

* Convert *LogicLab* libraries (`.h` ⇒ `.pll` ⇒ `.plclib`), more [below](#converting-to-library)
* Update libraries in a *LogicLab* project (`.ppjs`, `.plcprj`), more [below](#updating-a-project)

One of the main goals is to perform these tasks in the most efficient way:
this introduces some limitations on the accepted input files, mainly
related to using the raw content avoiding dynamic memory allocations
as much as possible.

_________________________________________________________________________
## Requirements

On windows:
* C++ runtime [`VC_redist.x64.exe`](https://aka.ms/vs/17/release/vc_redist.x64.exe).
* Parsing errors invoke [Notepad++](https://notepad-plus-plus.org)

On linux:
* Parsing errors invoke [mousepad](https://docs.xfce.org/apps/mousepad/start)


_________________________________________________________________________
## Usage

### Command line invocation

The most generic form:

```bat
$ lltool [task] [arguments in any order]
```

In a little more detail:

```bat
$ lltool [convert|update|help] [switches] [path(s)]
```

> [!TIP]
> * The first argument selects the task
> * The order of the subsequent arguments is not relevant
> * Some switches such as `--to` or `--options` require to be followed by a string
> * The switches can be passed in a brief notation using a single hyphen,
>   for example `-vF` is equivalent to `--verbose --force`.
> * The program won't overwrite any existing file unless you `--force` it.

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

To convert all `.h` files in `prog/` and all `.pll` files in `plc/`,
deleting existing files in the given output folder and
indicating some conversion options:

```bat
$ lltool convert prog/*.h plc/*.pll --options timestamp,plclib-indent:3 --force --to plc/LogicLab/generated-libs
```

> [!NOTE]
> * `.h` files will generate both `.pll` and `.plclib` files, while `.pll` files a `.plclib`
> * An error will be raised in case of output file names clashes
> * Use `--force` to clear the output directory content


To convert a single `.pll` file to a given output file:

```bat
$ lltool convert file.pll --force --to path/to/output.plclib
```
> [!IMPORTANT]
> The proper parsing and writing functions are selected by the file extensions


To update a project:

```bat
$ lltool update "C:\path\to\project.ppjs"
```
> [!CAUTION]
> The choice to backup or not the original file is yours: do it in your script.


To update a project without overwriting the original file:

```bat
$ lltool update "C:\path\to\project.ppjs" --force --to "C:\path\to\project-new.ppjs"
```

> [!TIP]
> Use `--force` to overwrite the output file if existing



_________________________________________________________________________
## Converting to library
The operation consists in translating the content of input files
into another format.

> [!NOTE]
> This operation offers no guarantee in case of ordinary runtime
> errors: an output file could be left incomplete and others may
> have been already written, leaving the set of libraries in a
> incoherent state.

> [!TIP]
> It is possible to convert multiple files at once
> specifying the paths and/or using glob patterns like `*.h`.
> In this case an output directory must be specified, and the
> operation will abort if finds conflicting output file names.


The supported conversions are:

* `.h` ⇒ `.pll`, `.plclib`
* `.pll` ⇒ `.plclib`

> [!TIP]
> * These will be the default conversions when the output file is not explicitly specified
> * File type is determined by its extension, mind the case

Where

* `.h` (*Sipro* header)
* `.pll` (Plc *LogicLab3* Library)
* `.plclib` (*LogicLab5* PLC LIBrary)

*Sipro* header files resemble a *c-like* header containing just
`#define` directives.
*LogicLab* libraries are *text* files embedding *IEC 61131-3* ST code;
the latest format (`.plclib`) embraces a pervasive *xml* structure.
More about these formats below.


### Conversion options
It is possible to specify a set of comma separated `key:value` pairs
to provide some control on the produced output.
The recognized keys are:

|   key              |    value            |               description               |
|--------------------|---------------------|-----------------------------------------|
| `timestamp`        |                     | Put a timestamp in generated file       |
| `sort`             |                     | Sort PLC elements and variables by name |
| `plclib-schemaver` | *\<uint\>.\<uint\>* | Schema version of generated plclib file |
| `plclib-indent`    | *\<uint\>*          | Tabs indentation of `<lib>` content     |

Example:

```bat
--options plclib-schemaver:2.8,plclib-indent:3,timestamp,sort
```


### Limitations
The following limitations are introduced to maximize efficiency:

* Input files must be encoded in `utf-8` and
  preferably use unix line breaks (`\n`)
* In windows, paths containing uppercase non ASCII characters may
  compromise name collision checks *(no lowercase converter for unicode)*
* Descriptions (`.h` `#define` inlined comments and `.pll` `{DE: ...}`)
  cannot contain XML special characters nor line breaks
* Check syntax limitations below


_________________________________________________________________________
### Syntax of *Sipro* header files
*Sipro* `.h` files supported syntax is:

```
// line comment
/*  -------------
    block comment
    ------------- */
#define reg_label register // description
#define val_label value // [type] description
```

> [!IMPORTANT]
> Inlined comments have meaning and define the resource description.

*Sipro* registers will be recognized and exported:

```cpp
#define vqWidth vq100 // Horizontal size
```

Numeric constants will be exported if the
desired *IEC 61131-3* type is explicitly
declared in the comment:

```cpp
#define PI 3.14159 // [LREAL] Circumference/diameter ratio
```

The recognized types are:

| type        | description                 | size | range                     |
| ----------- | --------------------------- | ---- | ------------------------- |
|   `BOOL`    | *BOOLean*                   |  1   | FALSE\|TRUE               |
|   `SINT`    | *Short INTeger*             |  1   | -128 … 127                |
|   `INT`     | *INTeger*                   |  2   | -32768 … 32767            |
|   `DINT`    | *Double INTeger*            |  4   | -2147483648 … 2147483647  |
| ~~`LINT`~~  | *Long INTeger*              |  8   | -2<sup>63</sup> … 2<sup>63</sup>-1 |
|   `USINT`   | *Unsigned Short INTeger*    |  1   | 0 … 255                   |
|   `UINT`    | *Unsigned INTeger*          |  2   | 0 … 65535                 |
|   `UDINT`   | *Unsigned Double INTeger*   |  4   | 0 … 4294967295            |
| ~~`ULINT`~~ | *Unsigned Long INTeger*     |  8   | 0 … 2<sup>64</sup>-1      |
| ~~`REAL`~~  | *REAL number*               |  4   | ±10<sup>38</sup>          |
|   `LREAL`   | *Long REAL number*          |  8   | ±10<sup>308</sup>         |
|   `BYTE`    | *1 byte*                    |  1   | 0 … 255                   |
|   `WORD`    | *2 bytes*                   |  2   | 0 … 65535                 |
|   `DWORD`   | *4 bytes*                   |  4   | 0 … 4294967295            |
| ~~`LWORD`~~ | *8 bytes*                   |  8   | 0 … 2<sup>64</sup>-1      |


_________________________________________________________________________
### Syntax of `.pll` files

Refer to *IEC 61131-3* `ST` syntax.
The parser, tested only on pure Structured Text projects, has the following limitations:

* Is case sensitive: recognizes only uppercase keywords (`PROGRAM`, ...)
* Not supported:
  * Persistent variables (`VAR RETAIN`) in *POU*s
  * Array values initialization (`ARRAY[0..1] OF INT := [1, 2];`)
  * Multidimensional arrays (`ARRAY[0..2, 0..2]`)
  * Pointers
  * Latest standard (`INTERFACE`, `THIS`, `PUBLIC`, `IMPLEMENTS`, `METHOD`, …)
  * *LogicLab* extension for conditional compilation `{IFDEF}`

Authors are encouraged to embed custom additional library data in
the first comment of the `.pll` file.
The recognized fields are `descr` and `version`, for example:

```
(*
    author: John Doe
    descr: Machine logic
    version: 1.2.31
*)
```



_________________________________________________________________________
## Updating a project
The operation consists in detecting the external linked
libraries (`<lib name="...">`) inside a project and updating
their content in the project file itself.
If the external libraries are stored in a different encoding,
their content will be embedded respecting the original project file
encoding.
> [!NOTE]
> This operation has a strong guarantee: in case of ordinary runtime errors
> the filesystem will be left in the same state as before the program invocation.


The supported project formats are:

|           |                       |
|-----------|-----------------------|
| `.ppjs`   | (*LogicLab3* project) |
| `.plcprj` | (*LogicLab5* project) |

The supported library formats are:

|           |                           |
|-----------|---------------------------|
| `.pll`    | (Plc *LogicLab3* Library) |
| `.plclib` | (*LogicLab5* PLC LIBrary) |

> [!IMPORTANT]
> File type is determined by its extension (must be lowercase)


### Limitations
The following limitations are introduced to maximize efficiency:

* Project files are assumed well formed
* Line breaks won't be converted: if the external libraries and
  the project file use a different convention,
  the resulting file will contain mixed line breaks
* plclib content won't be reindented (see `plclib-indent`)



_________________________________________________________________________
## Automated build of a Sipro LogicLab PLC project

Assuming this directory structure:

```
sde┐
   ├PROGS┐
   │     ├defvar.h
   │     ├<...other header files...>
   │     └···
   └PLC┐
       ├Machine.pll
       ├<...other pll files...>
       └LogicLab┐
                ├LogicLab.ppjs (main project file)
                └generated-libs
```

Here's a *powershell* script that performs a complete build
of the PLC using `lltool` to generate the libraries and
update the project file, and then invoking the compiler,
returning `0` if all steps completed successfully: 

```powershell
$libsoutdir = "PLC\LogicLab\generated-libs"
$ppjs = "PLC\LogicLab\LogicLab.ppjs"
#$plcprj = "PLC\LogicLab\LogicLab-plclib.plcprj"

function print
{
    param( [Parameter(Mandatory=$false)] [System.ConsoleColor]$color = [System.ConsoleColor]::White,
           [Parameter(Mandatory=$true)] [string]$message )

    Write-Host $message -ForegroundColor $color -NoNewline
}

function abort
{
    param( [Parameter(Mandatory=$true)] [string]$message,
           [Parameter(Mandatory=$false)] [int]$err_code = 2 )

    print Red "`n$message`n"
    pause
    exit $err_code
}

function check_exit_code
{
    param( [Parameter(Mandatory=$true)] [string]$who,
           [Parameter(Mandatory=$true)] [int]$ret_code )

    if( $ret_code -eq 0 )
       {
        print Green "$who [ok]`n"
       }
    else
       {
        abort "$who [err] ($ret_code)" $ret_code
       }
}

function get_plc_compiler_path
{
    $llc3 = "${env:ProgramFiles(x86)}\Sipro\Axel PC Tools\LogicLab3\LLC.exe"
    $llc5 = "${env:ProgramFiles(x86)}\Sipro\Siax PC Tools\LogicLab5\LLC.exe"
    if( Test-Path $llc3 )
       {
        return $llc3
       }
    elseif( Test-Path $llc5 )
       {
        return $llc5
       }
    else
       {
        abort "LogicLab not installed (LLC.exe not found)"
       }
}

function compile_plc_project
{
    param( [Parameter(Mandatory=$true)] [string]$proj_path )

    $proj_path = Resolve-Path $proj_path
    if( -not (Test-Path $proj_path) )
       {
        abort "$proj_path does not exist"
       }

    # [Update project]
    & lltool update `"$proj_path`"
    check_exit_code "lltool update" $LASTEXITCODE

    # [Compile project]
    $llc = get_plc_compiler_path
    $all_ok = $true
    & $llc `"$proj_path`" /r | ForEach-Object {
        # recognize "<num> warnings, <num> errors" in line $_
        if( $_ -match "(?i)(\d+)\s+warning[s]?\s*,?\s+(\d+)\s+error[s]?\b" )
           {
            $warn_count = [int]$matches[1]
            $err_count = [int]$matches[2]

            # Check if there are any warnings or errors
            if($warn_count -gt 0 -or $err_count -gt 0)
               {
                $all_ok = $false
                print Red "[ko] "
               }
            else
               {
                print Green "[ok] "
               }
           }
        print DarkGray ($_ + "`n")
       }

    check_exit_code "LLC" $LASTEXITCODE
    if( -not $all_ok )
       {
        abort "Compilation not clean!"
       }
    return $LASTEXITCODE -eq 0 -and $all_ok
}


# [Run in project directory]
Set-Location -Path "$PSScriptRoot\.."
if( -not (Test-Path "PLC") )
   {
    Write-Host (Get-Location)
    abort "No PLC folder, maybe running in the wrong directory?"
   }

# [Generate libraries]
& lltool convert PROG/*.h PLC/*.pll --options plclib-indent:3 --force --to $libsoutdir
check_exit_code "lltool convert" $LASTEXITCODE

$ppjs_compiled = compile_plc_project $ppjs
if( -not $ppjs_compiled )
   {
    abort "ppjs compilation error"
   }
   
#if( $plcprj )
#   {
#    $plcprj_compiled = compile_plc_project $plcprj
#    if( -not $plcprj_compiled )
#       {
#        abort "plcprj compilation error"
#       }
#   }
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
You need a `c++23` compliant toolchain.
Check the operations in the python script:

```sh
$ git clone https://github.com/matgat/lltool.git
$ cd lltool
$ python build/build.py
```

To run tests:

```sh
$ python test/run-all-tests.py
```


### linux
Launch `make` directly:

```sh
$ cd build
$ make
```

To run unit tests:

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

On Windows you need Microsoft Visual Studio 2022 (Community Edition).
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
