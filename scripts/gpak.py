"""List or extract files from Mewgenics resources.gpak (read-only on the archive).

Format: u32 count, then count x (u16 name_len, name, u32 size), then file data in index order.

    python scripts/gpak.py                      # list non-media files with sizes
    python scripts/gpak.py data/catgen.gon ...  # extract into extracted/<path>

As a module: `read_index()` -> {name: (offset, size)}, `read_file(name)` -> bytes.
"""
import os
import struct
import sys

GPAK = r"C:\Program Files (x86)\Steam\steamapps\common\Mewgenics\resources.gpak"
OUT = "extracted"


def read_index():
    with open(GPAK, "rb") as f:
        entries = []
        for _ in range(struct.unpack("<I", f.read(4))[0]):
            name = f.read(struct.unpack("<H", f.read(2))[0]).decode()
            entries.append((name, struct.unpack("<I", f.read(4))[0]))
        offset, index = f.tell(), {}
        for name, size in entries:
            index[name] = (offset, size)
            offset += size
    return index


def read_file(name, index=None):
    off, size = (index or read_index())[name]
    with open(GPAK, "rb") as f:
        f.seek(off)
        return f.read(size)


if __name__ == "__main__":
    index = read_index()
    if len(sys.argv) == 1:
        for name, (_, size) in index.items():
            if not name.endswith((".ogg", ".png", ".swf", ".wav")):
                print(size, name)
    for name in sys.argv[1:]:
        dest = os.path.join(OUT, name)
        os.makedirs(os.path.dirname(dest), exist_ok=True)
        with open(dest, "wb") as out:
            out.write(read_file(name, index))
        print("extracted", dest)
