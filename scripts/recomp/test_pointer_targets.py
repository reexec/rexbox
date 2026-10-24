"""pointer_targets.scan and merge_seeds on a small synthetic XBE (no game data)."""

import json
import pathlib
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from pointer_targets import merge_seeds, scan  # noqa: E402

BASE = 0x10000
TEXT_VA, TEXT_RAW, TEXT_SIZE = 0x11000, 0x1000, 0x100
DATA_VA, DATA_RAW, DATA_SIZE = 0x12000, 0x1100, 0x20


def fake_xbe():
    """A base-0x10000 image with .text and .data, laid out as XBE headers say."""
    image = bytearray(0x1200)
    struct.pack_into("<I", image, 0x104, BASE)
    struct.pack_into("<I", image, 0x11C, 2)
    struct.pack_into("<I", image, 0x120, BASE + 0x200)
    names = {".text": 0x300, ".data": 0x310}
    for i, (name, va, raw, size) in enumerate([(".text", TEXT_VA, TEXT_RAW, TEXT_SIZE),
                                              (".data", DATA_VA, DATA_RAW, DATA_SIZE)]):
        struct.pack_into("<6I", image, 0x200 + i * 56, 0, va, size, raw, size, BASE + names[name])
        image[names[name]:names[name] + len(name) + 1] = name.encode() + b"\0"
    text = bytearray(b"\xCC" * TEXT_SIZE)
    for start in (0x20, 0x40, 0x60):          # three function-shaped starts after int3 padding
        text[start] = 0x55                    # push ebp
    text[0x80] = 0x68                         # push offset 0x11040 (an immediate reference)
    struct.pack_into("<I", text, 0x81, TEXT_VA + 0x40)
    image[TEXT_RAW:TEXT_RAW + TEXT_SIZE] = text
    struct.pack_into("<II", image, DATA_RAW, TEXT_VA + 0x20, TEXT_VA + 0x60)  # vtable-like slots
    return bytes(image)


class Scan(unittest.TestCase):
    def test_candidates(self):
        references = scan(fake_xbe(), lifted={TEXT_VA + 0x60})
        self.assertEqual(sorted(references), [TEXT_VA + 0x20, TEXT_VA + 0x40])
        self.assertEqual(references[TEXT_VA + 0x20], [f".data {DATA_VA:08X}"])
        self.assertEqual(references[TEXT_VA + 0x40], [f"imm {TEXT_VA + 0x80:08X}"])

    def test_merge_seeds(self):
        with tempfile.TemporaryDirectory() as temp:
            seed_file = pathlib.Path(temp) / "seeds.json"
            seed_file.write_text(json.dumps([{"start": "0x00011040", "note": "by hand"}]), encoding="utf-8")
            merge_seeds(seed_file, {TEXT_VA + 0x20: [".data 00012000"], TEXT_VA + 0x40: ["imm 00011080"]})
            merged = json.loads(seed_file.read_text(encoding="utf-8"))
        self.assertEqual([entry["start"] for entry in merged], ["0x00011020", "0x00011040"])
        self.assertEqual(merged[1]["note"], "by hand")


if __name__ == "__main__":
    unittest.main()
