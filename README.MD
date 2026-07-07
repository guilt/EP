# Installer — Extractor/Packer for DOS & Windows

A cross-platform installer/packaging tool originally written in ~2001 \
to package the [3D Graphics Library (3DGFX)](https://github.com/guilt/3DGFX).
Rewritten as a C library with self-extracting archive (SFX) support.

This software comes with no warranty, implicit or explicit. Use at your own risk.

## Building

On **Windows** (MinGW-w64 / MSVC):
```
BUILD.BAT MINGW       MinGW-w64 GCC
BUILD.BAT MSVC        Microsoft Visual C++
```

On **DOS** (DOSBox-X etc.):
```
BUILD.BAT BCC         Borland C++
BUILD.BAT WCL         OpenWatcom C++
BUILD.BAT DJGPP       DJGPP GCC
```

## Usage

### Packing

Create a `.PAK` archive from an `.EPI` manifest:
```
PACK -C <dir> MANIFEST.EPI OUTPUT.PAK
```

Create a self-extracting `.EXE`:
```
PACK -S -X <EXTRACT.EXE> -C <dir> MANIFEST.EPI OUTPUT.EXE
```

Options:
```
  -C <dir>   Change to <dir> before processing manifest
  -S         Create self-extracting .EXE (prepends extractor stub)
  -X <exe>   Extractor binary to prepend for -S (default: ..\BIN\EXTRACT.EXE)
```

### Extracting

Extract a `.PAK` archive:
```
EXTRACT [-D <dir>] OUTPUT.PAK
```

Run an SFX (self-extracting) archive with no arguments:
```
OUTPUT.EXE
```

Options:
```
  -D <dir>   Extract into <dir> (default: archive name without extension)
```

After extraction, the installer prompts:
- **Run post-install commands** (e.g. launch a demo)
- **Delete the installer** (self-deletes SFX, or removes the `.PAK`)

## Manifest Format (.EPI)

The manifest describes what to include in the archive:

```
# Lines starting with # are comments
B <path>       Add banner text (displayed during extraction)
F <path>       Add file (supports * and ? wildcards)
D <dirname>    Add directory marker
C <desc>|<cmd> Add post-install command (user is prompted to run it)
```

Example (`3DGFX.EPI`):
```
# Installation banner
B ..\FILE_ID.DIZ

# Files
F ..\INCLUDE\TYPES.H
F ..\INCLUDE\FTARK*.H

# Directory
D docs

# Post-install command
C Run Demo|RUNDEMO.BAT
```

## Archive Format (PF)

The `.PAK` format uses a simple binary layout (16-bit compatible):

```
Offset  Size  Field
0       2     Magic "PF"
2       2     uint16  entryCount
4+      ...   entries
```

Each entry:
```
Offset  Size  Field
0       2     uint16  nameLen
nameLen char   name[]
2       2     uint16  dflag   (0=file, 1=dir, 2=banner, 3=command)
if dflag == 0 or 2:
  0     4     uint32  contentLen
  contentLen  char    content[]
```

## Project Structure

```
SRC\
  INSTALL.C     Library (context, pack, extract, wildcard, copy)
  PACK.C        CLI packer
  EXTRACT.C     CLI extractor

INCLUDE\
  INSTALL.H     Public API + platform macros
  VER.H         Version info

RC\
  MAIN.RC       Windows icon resource
  ICON1.ICO     Multi-resolution icon (16/32/64)

OBJ\            Intermediate object files
BIN\            Final executables
```

## License

[MIT License](LICENSE.MD)

## Acknowledgments

Original EP (Extractor Packer) Installer by Karthik Kumar Viswanathan. 
Deepseek did the library rewrite.  
Gemini AI generated the application icon.  
Inspired by the original 3DGFX packaging system.

*Author: Karthik Kumar Viswanathan*  
*Web: https://karthikkumar.org*  
*Email: me@karthikkumar.org*
