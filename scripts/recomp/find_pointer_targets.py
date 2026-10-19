#!/usr/bin/env python3
"""List game functions that are reached only through pointers and were not lifted.

The disassembler follows direct calls and jumps. A function whose address only
appears as data (a C++ vtable slot in .rdata/.data) or as an immediate (a
thread routine passed with `push offset fn`) is never discovered, and calling
it at run time ends in "indirect call ... has no target; skipped".

This scan finds such addresses in the game's .text section by the shape the
compiler gives its functions: 16-byte aligned, preceded by nop/int3 padding or
a return, and not starting with padding itself. Addresses already lifted (in
the generated dispatch table) are left out. XDK library sections are not
scanned: their functions are not aligned, and the libraries are HLE or seeded
by hand.

Usage:
    python find_pointer_targets.py --xbe <xbe> --variant <v> --variants <variants.json>
                                   --dispatch <gen>/gen/recomp_dispatch.c --seed-file <json> [--write]

--write replaces the seed file, which regenerate.py reads like any other seed
file. Rerun regenerate.py afterwards. A title's wrapper fills in the paths.
"""

import argparse
import json
import pathlib
import re
import struct
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from xbe_variant import check_xbe  # noqa: E402

PADDING = (0x90, 0xCC)


def sections(image):
    base = struct.unpack_from("<I", image, 0x104)[0]
    count = struct.unpack_from("<I", image, 0x11C)[0]
    headers = struct.unpack_from("<I", image, 0x120)[0] - base
    result = []
    for i in range(count):
        _, va, _, raw, raw_size, name_address = struct.unpack_from("<6I", image, headers + i * 56)
        name_offset = name_address - base
        name = image[name_offset:image.index(b"\0", name_offset)].decode()
        result.append((name, va, raw, raw_size))
    return result


def build_parser(defaults):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--xbe", type=pathlib.Path, default=defaults.get("xbe"))
    parser.add_argument("--variant", default=defaults.get("variant"))
    parser.add_argument("--variants", type=pathlib.Path, default=defaults.get("variants"))
    parser.add_argument("--dispatch", type=pathlib.Path, default=defaults.get("dispatch"),
                        help="the generated recomp_dispatch.c")
    parser.add_argument("--seed-file", type=pathlib.Path, default=defaults.get("seed_file"))
    parser.add_argument("--write", action="store_true")
    return parser


def run(argv=None, **defaults):
    parser = build_parser(defaults)
    args = parser.parse_args(argv)
    for required in ("xbe", "variant", "variants", "dispatch", "seed_file"):
        if getattr(args, required) is None:
            parser.error(f"--{required.replace('_', '-')} is required")

    problem = check_xbe(args.xbe, args.variant, args.variants)
    if problem:
        print(f"error: {problem}", file=sys.stderr)
        return 2
    image = pathlib.Path(args.xbe).read_bytes()
    if not args.dispatch.exists():
        print(f"error: {args.dispatch} is missing; run regenerate.py first", file=sys.stderr)
        return 2
    lifted = {int(value, 16) for value in re.findall(r"\{ 0x([0-9A-Fa-f]{8})u,", args.dispatch.read_text())}

    layout = sections(image)
    text = next(s for s in layout if s[0] == ".text")
    _, text_va, text_raw, text_size = text

    def byte_at(address):
        return image[text_raw + address - text_va]

    def looks_like_start(address):
        if not (text_va + 3 <= address < text_va + text_size) or address % 16 != 0 or address in lifted:
            return False
        if byte_at(address) in PADDING:
            return False
        before = byte_at(address - 1)
        return before in PADDING or before == 0xC3 or byte_at(address - 3) == 0xC2

    references = {}
    for name, va, raw, size in layout:
        if name not in (".rdata", ".data"):
            continue
        for offset in range(0, size - 3, 4):
            value = struct.unpack_from("<I", image, raw + offset)[0]
            if looks_like_start(value):
                references.setdefault(value, []).append(f"{name} {va + offset:08X}")
    code = image[text_raw:text_raw + text_size]
    for offset in range(len(code) - 5):
        opcode = code[offset]
        if opcode == 0x68 or 0xB8 <= opcode <= 0xBF:  # push imm32, mov r32, imm32
            value = struct.unpack_from("<I", code, offset + 1)[0]
            if looks_like_start(value):
                references.setdefault(value, []).append(f"imm {text_va + offset:08X}")

    for address in sorted(references):
        where = references[address]
        print(f"{address:08X}  {', '.join(where[:3])}{' ...' if len(where) > 3 else ''}")
    print(f"{len(references)} candidate(s)")

    if args.write:
        seeds = [
            {"start": f"0x{address:08X}", "note": f"pointer-only target ({references[address][0]})"}
            for address in sorted(references)
        ]
        previous = []
        if args.seed_file.exists():
            previous = json.loads(args.seed_file.read_text(encoding="utf-8"))
        known = {entry["start"] for entry in previous}
        merged = previous + [entry for entry in seeds if entry["start"] not in known]
        merged.sort(key=lambda entry: int(entry["start"], 16))
        lines = [json.dumps(entry) for entry in merged]
        args.seed_file.write_text("[\n  " + ",\n  ".join(lines) + "\n]\n", encoding="utf-8", newline="\n")
        print(f"wrote {args.seed_file} ({len(merged)} seed(s))")
    return 0


if __name__ == "__main__":
    sys.exit(run())
