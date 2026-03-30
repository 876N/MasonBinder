# MasonBinder v2.0
### Professional File Binder
**(c) 2026 MasonGroup - All Rights Reserved**

![Tool](https://i.ibb.co/cKFSv0bR/Binder.png)

---

## Overview

MasonBinder is a professional file binding utility that combines multiple files into a single self-extracting executable (.exe). Built with pure Win32 API in C for maximum performance and a classic light UI

---

## Features

| Feature | Description |
|---|---|
| Self-Extracting EXE | Generates a standalone .exe that extracts and runs files |
| RLE Compression | Built-in Run-Length Encoding compression engine |
| Per-File Drop Paths | Each file can have its own extraction path (AppData, Temp, Current, Custom) |
| Execution Modes | Running, Runonce (first launch only), No Run |
| Startup Persistence | Registry Run key or Task Scheduler persistence |
| Hidden Extraction | Extract files with Hidden + System attributes |
| Sleep Delay | Per-file configurable delay before execution |
| Icon Embedding | Set a custom .ico icon for the output executable |
| UAC Manifest | Inject requireAdministrator or asInvoker manifest |
| File Size Pump | Increase output file size with zero-padding |
| Self-Delete (Melt) | Output EXE deletes itself after extraction |
| Drag & Drop | Drop files directly onto the window |
| CRC32 Verification | Integrity checking during extraction |
| Up to 512 Files | Bind up to 512 files in a single executable |

---

## Project Structure

```
MasonBinder/
  MasonBinder.c      Main application (UI + build logic)
  MasonStub.c        Self-extracting stub engine (embedded in output EXE)
  MasonGenStub.c     Utility to convert MasonStub.exe into a C header
  icon.rc            Resource file for application icon
  icon.ico           Application icon
  README.md          This file
```

---

## Build Instructions

### Requirements

- **MinGW / GCC** (with Windows SDK headers)
- **windres** (part of MinGW, for compiling icon resource)

### Step-by-Step Build

**1. Compile the Stub (self-extracting engine):**

```
gcc -O2 -s -mwindows -o MasonStub.exe MasonStub.c -lshell32 -lole32 -luser32 -ladvapi32
```

**2. Generate the Stub header file:**

```
gcc -O2 -o MasonGenStub.exe MasonGenStub.c
MasonGenStub.exe MasonStub.exe MasonStubData.h
```

This converts MasonStub.exe into a C byte array header (`MasonStubData.h`) that gets embedded into MasonBinder

**3. Compile the icon resource:**

```
windres icon.rc -o icon.o
```

**4. Compile MasonBinder:**

```
gcc -O2 -s -mwindows -o MasonBinder.exe MasonBinder.c icon.o -lcomctl32 -lcomdlg32 -lshell32 -lole32 -lgdi32 -luser32 -ladvapi32
```

**5. Cleanup temporary files (optional):**

```
del MasonGenStub.exe MasonStub.exe icon.o MasonStubData.h
```

### One-Liner Build (copy-paste)

```
gcc -O2 -s -mwindows -o MasonStub.exe MasonStub.c -lshell32 -lole32 -luser32 -ladvapi32 && gcc -O2 -o MasonGenStub.exe MasonGenStub.c && MasonGenStub.exe MasonStub.exe MasonStubData.h && windres icon.rc -o icon.o && gcc -O2 -s -mwindows -o MasonBinder.exe MasonBinder.c icon.o -lcomctl32 -lcomdlg32 -lshell32 -lole32 -lgdi32 -luser32 -ladvapi32 && del MasonGenStub.exe MasonStub.exe icon.o
```

---

## Usage

1. Launch `MasonBinder.exe`
2. Right-click the list area and select **Add Files** or drag files onto the window
3. Right-click any file to configure: Drop Path, Execution Mode, Startup, Hidden, Sleep
4. Set global options at the bottom: Drop location, Compress, Hidden, UAC, Melt, Pump
5. Optionally click **Icon...** to set a custom icon for the output EXE
6. Click **BUILD EXE** to generate the self-extracting executable

---

## Right-Click Menu Options

| Option | Description |
|---|---|
| Add Files... | Browse and add files to the list |
| Remove | Remove selected file from list |
| Move Up / Down | Reorder file execution priority |
| Drop Path | Set per-file extraction path (AppData, Temp, Current, Custom) |
| Execution | Running / Runonce / No Run |
| Startup | None / Registry / Scheduler persistence |
| Hidden | Toggle hidden file attribute |
| Sleep... | Set delay in milliseconds before execution |
| Reset to Default | Reset all per-file settings |
| Clear All | Remove all files from list |

---

## Archive Format

The output EXE has this internal structure:

```
+---------------------------+
| MasonStub.exe (PE)        |  Stub engine
+---------------------------+
| Icon + Manifest resources |  Optional (embedded via UpdateResource)
+---------------------------+
| MASON_ARCHIVE_HEADER      |  Magic, version, file count, flags
+---------------------------+
| MASON_FILE_ENTRY[0..N-1]  |  Per-file metadata (name, size, flags, drop path)
+---------------------------+
| File Data Block 0..N-1    |  Raw or RLE-compressed file data
+---------------------------+
| Pump Data (optional)      |  Zero-padding for size increase
+---------------------------+
| MASON_STUB_FOOTER         |  Archive offset, global drop path, flags, magic
+---------------------------+
```

---

## License

This software is proprietary to MasonGroup
Unauthorized reproduction or distribution is prohibited

(c) 2026 MasonGroup Engineering Division
