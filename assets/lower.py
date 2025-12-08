#!/usr/bin/env python3
import os

root = "."  # muda para a tua pasta de texturas

for dirpath, dirnames, filenames in os.walk(root):
    for filename in filenames:
        low = filename.lower()
        if filename != low:
            old = os.path.join(dirpath, filename)
            new = os.path.join(dirpath, low)
            print(f"{old} -> {new}")
            os.rename(old, new)

