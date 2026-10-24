#!/usr/bin/env python3
"""Compare each XDK HLE export's stack pop count with the original's `ret n`.

An HLE export returns through HleCall::Return(value, n), popping n stack
arguments. If n differs from the original function's `ret n`, the caller's
stack is skewed and the guest runs off into unrelated code with no error at
the call site. The HLE sources carry n in their Return calls; the original's
`ret n` comes from the title's lifted code, which only the title can read. So
a title's own check reads the lifted bodies and calls this:

    names = xdk_function_names(<gen>/xdk_symbols.json, <title>/xdk_symbols_extra.json)
    lines, mismatches = compare_pops(names, lifted_rets, hle_pop_counts())

`lifted_rets` maps a function's guest address to the operands of the `ret`
instructions in its lifted body, in bytes (0 for a plain `ret`). An address
with no entry is reported as "no ret in lifted body": a function whose
original falls through into another function or has an empty body (inferred
names), checked by hand.
"""

import glob
import json
import pathlib
import re

REXBOX = pathlib.Path(__file__).resolve().parents[2]
HLE_DIRS = [REXBOX / "src" / "hle" / name for name in ("d3d8", "dsound", "xinput")]


def xdk_function_names(symbols_json, extra_symbols=None):
    """{guest address: XDK function name} from rexbox_xdk_symbols' output and a title's extra symbols."""
    symbols = json.loads(pathlib.Path(symbols_json).read_text(encoding="utf-8"))
    extra = []
    if extra_symbols is not None and pathlib.Path(extra_symbols).is_file():
        extra = json.loads(pathlib.Path(extra_symbols).read_text(encoding="utf-8"))["symbols"]
    name_by_address = {}
    for symbol in symbols + extra:
        if symbol["kind"] == "function":
            name_by_address.setdefault(int(symbol["address"], 16), symbol["name"])
    return name_by_address


def hle_pop_counts(hle_dirs=None):
    """{export name: set of pop counts} read from the Return calls of rexbox's HLE sources."""
    hle = {}
    for path in [p for d in (hle_dirs or HLE_DIRS) for p in glob.glob(str(d / "*.cpp"))]:
        hle.update(pop_counts_in(pathlib.Path(path).read_text(encoding="utf-8")))
    return hle


def pop_counts_in(source):
    """The exports one HLE source registers, with the pop counts of their Return calls."""
    functions = {}
    for match in re.finditer(r"^(?:template <[^>]*>\n)?void (\w+)\(\)\n\{(.*?)^\}", source, re.S | re.M):
        pops = set(int(x) for x in re.findall(r"Return(?:64)?\([^;]*?,\s*(\d+)\);", match.group(2)))
        functions[match.group(1)] = pops
    hle = {}
    for match in re.finditer(r'AddExport\("\w+", "(\w+)", &(\w+)(?:<(\d+)>)?\)', source):
        pops = functions.get(match.group(2), set())
        if not pops and match.group(3) is not None:
            # A template whose argument is the pop count (dsound's AcceptSetting<N>).
            pops = {int(match.group(3))}
        hle[match.group(1)] = pops
    return hle


def compare_pops(name_by_address, lifted_rets, hle, show_all=False):
    """(report lines, mismatch count). Matches are listed only with `show_all`."""
    lines = []
    mismatches = 0
    for address, name in sorted(name_by_address.items(), key=lambda item: item[1]):
        if name not in hle:
            continue
        lifted = lifted_rets.get(address)
        pops = hle[name]
        if not lifted:
            lines.append(f"{name:48s} no ret in lifted body; HLE pops {sorted(pops)} (check by hand)")
            continue
        expected = {value // 4 for value in lifted}
        if not pops:
            lines.append(f"{name:48s} lifted ret {lifted}; HLE pops not parsed (helper function?)")
            continue
        ok = pops == expected
        if not ok:
            mismatches += 1
        if show_all or not ok:
            lines.append(f"{name:48s} lifted ret {lifted} -> {sorted(expected)} args; HLE pops {sorted(pops)}"
                         f"{'' if ok else '   <-- MISMATCH'}")
    lines.append(f"{mismatches} mismatch(es)")
    return lines, mismatches
