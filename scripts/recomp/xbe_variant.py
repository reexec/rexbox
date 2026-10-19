#!/usr/bin/env python3
"""Identity of an XBE, and the check that it is the build a variant is lifted from.

A title's seeds, title bindings and decompiled functions hold addresses of one
exact build, named in the title's variants file:

    {"variants": {"<variant>": {"label": "...", "md5": "<md5 of default.xbe>", ...}}}

Lifting another build would silently produce code that does not match them,
so the recompilation scripts stop instead. A variant whose "md5" is null is
not registered yet: `python xbe_variant.py <xbe>` prints what to put there
(the MD5 and the certificate fields a title profile identifies the build by).

Usage:
    python xbe_variant.py <default.xbe>                 # print the identity
    python xbe_variant.py <default.xbe> --json          # as JSON
"""

import argparse
import hashlib
import json
import pathlib
import struct
import sys


def xbe_identity(path):
    """The fields a title profile and a variants file identify a build by."""
    data = pathlib.Path(path).read_bytes()
    if data[:4] != b"XBEH":
        raise ValueError(f"{path} is not an XBE (no XBEH magic)")
    base = struct.unpack_from("<I", data, 0x104)[0]
    image_time = struct.unpack_from("<I", data, 0x114)[0]
    certificate = struct.unpack_from("<I", data, 0x118)[0] - base
    title_id = struct.unpack_from("<I", data, certificate + 0x8)[0]
    title_name = data[certificate + 0xC:certificate + 0xC + 80].decode("utf-16-le", "replace").rstrip("\0")
    certificate_time = struct.unpack_from("<I", data, certificate + 0x4)[0]
    region = struct.unpack_from("<I", data, certificate + 0xA0)[0]
    version = struct.unpack_from("<I", data, certificate + 0xAC)[0]
    return {
        "md5": hashlib.md5(data).hexdigest(),
        "size": len(data),
        "title_id": f"0x{title_id:08X}",
        "title_name": title_name,
        "certificate_time": f"0x{certificate_time:08X}",
        "certificate_version": f"0x{version:08X}",
        "image_time": f"0x{image_time:08X}",
        "region": f"0x{region:08X}",
    }


def check_xbe(xbe, variant, variants_path):
    """Returns None when `xbe` is `variant`'s build, otherwise the reason it is not."""
    variants = json.loads(pathlib.Path(variants_path).read_text(encoding="utf-8"))["variants"]
    if variant not in variants:
        return f"unknown variant {variant} (known: {', '.join(sorted(variants))})"
    expected = variants[variant]
    if not expected.get("md5"):
        return (f"variant {variant} ({expected.get('label', '?')}) is not registered yet in {variants_path}: "
                f"run the title's scripts/recomp/register_xbe.py (or xbe_variant.py {xbe}) and fill in its md5")
    actual = hashlib.md5(pathlib.Path(xbe).read_bytes()).hexdigest()
    if actual != expected["md5"]:
        return (f"{xbe} is not the {expected['label']} build that {variant} is lifted from "
                f"(MD5 {actual}, expected {expected['md5']}); pass that build with --xbe <path>")
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("xbe", type=pathlib.Path)
    parser.add_argument("--json", action="store_true", help="print the identity as JSON")
    args = parser.parse_args()
    identity = xbe_identity(args.xbe)
    if args.json:
        print(json.dumps(identity, indent=2))
    else:
        for key, value in identity.items():
            print(f"{key:20s} {value}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
