"""List or extract files from Mewgenics resources.gpak (read-only on the archive).

Format: u32 count, then count x (u16 name_len, name, u32 size), then file data in index order.

    python scripts/gpak.py                      # list non-media files with sizes
    python scripts/gpak.py data/catgen.gon ...  # extract into extracted/<path>
"""
import os
import struct
import sys

GPAK = r"C:\Program Files (x86)\Steam\steamapps\common\Mewgenics\resources.gpak"
OUT = "extracted"

with open(GPAK, "rb") as f:
    entries = []
    for _ in range(struct.unpack("<I", f.read(4))[0]):
        name = f.read(struct.unpack("<H", f.read(2))[0]).decode()
        entries.append((name, struct.unpack("<I", f.read(4))[0]))
    offset, index = f.tell(), {}
    for name, size in entries:
        index[name] = (offset, size)
        offset += size

    if len(sys.argv) == 1:
        for name, size in entries:
            if not name.endswith((".ogg", ".png", ".swf", ".wav")):
                print(size, name)
    for name in sys.argv[1:]:
        off, size = index[name]
        f.seek(off)
        dest = os.path.join(OUT, name)
        os.makedirs(os.path.dirname(dest), exist_ok=True)
        with open(dest, "wb") as out:
            out.write(f.read(size))
        print("extracted", dest)
