#!/usr/bin/env python3
"""Fail if any header file sits under src/libraries/.

Library headers, public and private, belong in src/common/<library>/.
src/libraries/ holds only .c sources and meson.build files. Binaries in
src/binaries/ are self-contained and keep their own headers, so they are
not checked.
"""
import pathlib
import sys

root = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else pathlib.Path.cwd()
libraries = root / "src" / "libraries"
misplaced = sorted(p for p in libraries.rglob("*") if p.suffix in (".h", ".hpp"))

if misplaced:
    for path in misplaced:
        rel = path.relative_to(libraries)
        print(f"{path.relative_to(root)}: library headers belong in "
              f"src/common/{rel.as_posix()}, not src/libraries/")
    sys.exit(1)
print("header layout OK")
