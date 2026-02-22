#!/usr/bin/env python3
"""
Create test archives for archive system tests.
This script generates the required zip files with the proper structure.
"""

import os
import zipfile
from pathlib import Path

# Get the directory where this script is located
script_dir = Path(__file__).parent
archivedir = script_dir / "archivedir"

print(f"Creating test archives in: {archivedir}")

# Ensure archivedir exists
archivedir.mkdir(exist_ok=True)

# Create archivetest.zip
archivetest_path = archivedir / "archivetest.zip"
print(f"\nCreating {archivetest_path.name}...")

with zipfile.ZipFile(archivetest_path, 'w', zipfile.ZIP_DEFLATED) as zf:
    # Add test.txt at root level (13 bytes: "asdfadasdfasd\n")
    test_txt_content = b"asdfadasdfasd\n"
    zf.writestr("test.txt", test_txt_content)
    print(f"  Added test.txt ({len(test_txt_content)} bytes)")
    
    # Add lol/ directory and contents
    # Add lol/tar/new.txt (16 bytes: "lolpoisonutrypop\n")
    new_txt_content = b"lolpoisonutrypop\n"
    zf.writestr("lol/tar/new.txt", new_txt_content)
    print(f"  Added lol/tar/new.txt ({len(new_txt_content)} bytes)")

print(f"  Created: {archivetest_path}")

# Create archivedir.zip (archive containing another archive)
archivedir_zip_path = archivedir / "archivedir.zip"
print(f"\nCreating {archivedir_zip_path.name}...")

with zipfile.ZipFile(archivedir_zip_path, 'w', zipfile.ZIP_DEFLATED) as zf:
    # Add the archivetest.zip file inside this archive
    zf.write(archivetest_path, arcname="archivetest.zip")
    print(f"  Added archivetest.zip (nested archive)")

print(f"  Created: {archivedir_zip_path}")

print("\n✓ Test archives created successfully!")
print(f"\nArchive contents:")
print(f"\n{archivetest_path.name}:")
with zipfile.ZipFile(archivetest_path, 'r') as zf:
    for info in zf.filelist:
        print(f"  {info.filename} ({info.file_size} bytes)")

print(f"\n{archivedir_zip_path.name}:")
with zipfile.ZipFile(archivedir_zip_path, 'r') as zf:
    for info in zf.filelist:
        print(f"  {info.filename} ({info.file_size} bytes)")
