#!/usr/bin/env python3
"""Regenerate the lifted C for one XBE variant of a title.

Runs the xboxrecomp pipeline on the user's own XBE and writes everything under
<gen root>/<variant>/, which a title gitignores: the generated code is derived
from the game's bytes and is never committed.

    <gen>/analysis.json      XBE layout
    <gen>/xdk_symbols.json   XDK functions found by rexbox_xdk_symbols
    <gen>/disasm/            functions, xrefs, strings
    <gen>/func-id/           function classification
    <gen>/abi/               calling conventions
    <gen>/hle_overrides.c    overrides routing XDK functions to the HLE
    <gen>/overrides.c        everything the lifter must not emit as is
    <gen>/gen/               lifted C, built by rexbox_add_recompiled_library

A title calls this through its own scripts/recomp/regenerate.py, which fills
in the paths below (see `run`); every one can also be given on the command
line.

The title's recomp directory (--title-dir) holds:
    variants.json            the XBE each variant is lifted from (md5)
    *.json lists             function entry points the disassembler misses
                             ({"start": "0x..."}); each is passed as a seed
    hle_libraries.json       libraries/functions replaced by HLE exports
    xdk_symbols_extra.json   XDK functions XbSymbolDatabase misses

HLE overrides: rexbox_xdk_symbols (XbSymbolDatabase) identifies the XDK
library functions in the XBE. For every function of a library named in
hle_libraries.json, an override of its lifted symbol is generated that
enters the HLE through rexbox_hle_enter (include/rexbox/recomp/recomp_hle_entry.h).

Decompiled overrides: every `void sub_XXXXXXXX(void)` defined under the
override directories (--override-dir, the title's decomp/) is handed to the
lifter as --exclude-manual, so direct calls and the generated dispatch table
link to the hand-written version. A file that also declares
`extern void sub_XXXXXXXX_gen(void);` keeps the lifted body under that name.

The XBE must be the exact build the variant is lifted from (its MD5 is in
variants.json); any other build stops the run.
"""

import argparse
import json
import pathlib
import shutil
import subprocess
import sys
import time

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from xbe_variant import check_xbe  # noqa: E402

REXBOX = pathlib.Path(__file__).resolve().parents[2]


def run_step(name, xboxrecomp, args):
    print(f"=== {name}", flush=True)
    started = time.monotonic()
    result = subprocess.run([sys.executable, "-m"] + [str(a) for a in args], cwd=xboxrecomp)
    if result.returncode != 0:
        sys.exit(f"{name} failed with exit code {result.returncode}")
    print(f"    {name}: {time.monotonic() - started:.1f}s", flush=True)


def seed_files(title_dir):
    """Seed files are the JSON lists in the title's recomp directory."""
    seeds = []
    for path in sorted(title_dir.glob("*.json")) if title_dir.is_dir() else []:
        if isinstance(json.loads(path.read_text(encoding="utf-8")), list):
            seeds.append(path)
    return seeds


def find_symbol_tool(explicit, candidates):
    if explicit is not None:
        return pathlib.Path(explicit).resolve()
    for candidate in candidates:
        if pathlib.Path(candidate).is_file():
            return pathlib.Path(candidate)
    sys.exit("rexbox_xdk_symbols not found -- build it first "
             "(scripts/build_win32_x86.ps1 -Target rexbox_xdk_symbols) or pass --symbols-tool")


def identify_symbols(tool, xbe, out_dir, extra_symbols):
    print("=== xdk symbols", flush=True)
    target = out_dir / "xdk_symbols.json"
    result = subprocess.run([str(tool), str(xbe), "--json", str(target)], capture_output=True, text=True)
    if result.returncode != 0:
        sys.exit(f"rexbox_xdk_symbols failed: {result.stderr.strip()}")
    symbols = json.loads(target.read_text(encoding="utf-8"))
    if extra_symbols is not None and extra_symbols.is_file():
        known = {int(s["address"], 16) for s in symbols}
        for extra in json.loads(extra_symbols.read_text(encoding="utf-8"))["symbols"]:
            if int(extra["address"], 16) not in known:
                symbols.append(dict(extra, call="unknown", params=0, build=0))
    print(f"    {len(symbols)} symbol(s)")
    return symbols


def write_hle_overrides(symbols, functions_json, out_dir, hle_libraries):
    """One override per function of an HLE library that the disassembler found."""
    config = json.loads(hle_libraries.read_text(encoding="utf-8")) if hle_libraries.is_file() else {}
    libraries = set(config.get("libraries", []))
    single = {(f["library"], f["name"]) for f in config.get("functions", [])}
    starts = {int(f["start"], 16) for f in json.loads(functions_json.read_text(encoding="utf-8"))}
    chosen = {}
    skipped = []
    for symbol in symbols:
        if symbol["kind"] != "function":
            continue
        if symbol["library"] not in libraries and (symbol["library"], symbol["name"]) not in single:
            continue
        address = int(symbol["address"], 16)
        if address not in starts:
            skipped.append(f"{symbol['library']} {symbol['name']} 0x{address:08X}")
            continue
        chosen.setdefault(address, symbol)  # first name wins for aliases

    lines = [
        "/* Generated by rexbox scripts/recomp/regenerate.py from xdk_symbols.json. Not committed.",
        " * Each function of an HLE library enters rexbox_hle_enter; the lifted body",
        " * stays available as sub_X_gen. */",
        "",
        "#include \"rexbox/recomp/recomp_hle_entry.h\"",
        "",
    ]
    for address, symbol in sorted(chosen.items()):
        tag = f"{address:08X}"
        lines += [
            f"extern void sub_{tag}_gen(void);",
            f"static RexboxHleSlot slot_{tag} = {{0x{tag}u, \"{symbol['library']}\", \"{symbol['name']}\", sub_{tag}_gen, 0, 0}};",
            f"void sub_{tag}(void)",
            "{",
            f"    rexbox_hle_enter(&slot_{tag});",
            "}",
            "",
        ]
    # Identified XDK variables, for HLE modules that receive their addresses
    # (XInput's device types are &g_DeviceType_*). Absolute-looking values below
    # the image are structure offsets the database reports as variables; they
    # are kept, the binding is by name.
    variables = sorted({(s["library"], s["name"], int(s["address"], 16))
                        for s in symbols if s["kind"] == "variable"}, key=lambda v: (v[2], v[1]))
    lines += [
        "const RexboxXdkVariable rexbox_xdk_variables[] = {",
    ] + [f"    {{\"{lib}\", \"{name}\", 0x{addr:08X}u}}," for lib, name, addr in variables] + [
        "    {0, 0, 0},",
        "};",
        "",
    ]
    missing = sorted(single - {(s["library"], s["name"]) for s in chosen.values()})
    for library, name in missing:
        skipped.append(f"{library} {name} (selected function not identified)")
    target = out_dir / "hle_overrides.c"
    target.write_text("\n".join(lines), encoding="utf-8", newline="\n")
    print(f"    hle overrides: {len(chosen)} function(s) of {sorted(libraries)} and {len(single)} selected; "
          f"{len(skipped)} skipped; {len(variables)} XDK variable(s)")
    for item in skipped:
        print(f"      skipped {item}")
    return target


def collect_overrides(out_dir, hle_overrides, override_dirs, root):
    """Concatenate the HLE overrides and every decompiled source for the lifter's scan."""
    sources = sorted(p for d in override_dirs if d.is_dir() for p in d.rglob("*.c"))
    target = out_dir / "overrides.c"
    with target.open("w", encoding="utf-8", newline="\n") as out:
        out.write("/* Generated by rexbox scripts/recomp/regenerate.py: scan input only, not compiled. */\n")
        for source in [hle_overrides] + sources:
            try:
                label = source.relative_to(root).as_posix()
            except ValueError:
                label = source.as_posix()
            out.write(f"\n/* ---- {label} ---- */\n")
            out.write(source.read_text(encoding="utf-8"))
    print(f"    overrides: HLE + {len(sources)} decompiled source file(s)")
    return target


def check_checkout_clean(xboxrecomp):
    status = subprocess.run(["git", "-C", str(xboxrecomp), "status", "--porcelain"],
                            capture_output=True, text=True)
    if status.stdout.strip():
        print(f"WARNING: the pipeline left files inside {xboxrecomp}:", file=sys.stderr)
        print(status.stdout, file=sys.stderr)


def build_parser(defaults):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--xbe", type=pathlib.Path, default=defaults.get("xbe"))
    parser.add_argument("--variant", default=defaults.get("variant"), help="variant name in variants.json")
    parser.add_argument("--title-dir", type=pathlib.Path, default=defaults.get("title_dir"),
                        help="the title's recomp configuration directory (variants.json, seeds, hle_libraries.json)")
    parser.add_argument("--override-dir", type=pathlib.Path, action="append", default=None,
                        help="directory of decompiled sources (repeatable; default: the title's decomp/)")
    parser.add_argument("--gen-root", type=pathlib.Path, default=defaults.get("gen_root"),
                        help="generated code goes to <gen root>/<variant>/")
    parser.add_argument("--xboxrecomp", type=pathlib.Path,
                        default=defaults.get("xboxrecomp", REXBOX / "third_party" / "xboxrecomp"),
                        help="xboxrecomp checkout whose tools lift the code")
    parser.add_argument("--symbols-tool", type=pathlib.Path, default=None,
                        help="path to rexbox_xdk_symbols (default: look in the build directories)")
    parser.add_argument("--split", type=int, default=250,
                        help="functions per generated file (smaller files compile in parallel)")
    return parser


def run(argv=None, **defaults):
    """Entry point for a title's wrapper: `run(xbe=..., variant=..., title_dir=..., gen_root=..., ...)`.

    Extra defaults: `override_dirs` (list of paths), `symbol_tool_candidates`
    (list of paths), `root` (the title's repository root, for messages)."""
    parser = build_parser(defaults)
    args = parser.parse_args(argv)
    for required in ("xbe", "variant", "title_dir", "gen_root"):
        if getattr(args, required) is None:
            parser.error(f"--{required.replace('_', '-')} is required")
    root = pathlib.Path(defaults.get("root", args.title_dir)).resolve()
    override_dirs = args.override_dir or list(defaults.get("override_dirs", []))
    xboxrecomp = args.xboxrecomp.resolve()

    xbe = args.xbe.resolve()
    if not xbe.is_file():
        sys.exit(f"no XBE at {xbe} -- put your own dump under roms/ (docs/guides/)")
    problem = check_xbe(xbe, args.variant, args.title_dir / "variants.json")
    if problem:
        sys.exit(f"error: {problem}")
    if not (xboxrecomp / "tools").is_dir():
        sys.exit(f"{xboxrecomp} is empty -- run: git submodule update --init")
    tool = find_symbol_tool(args.symbols_tool, defaults.get("symbol_tool_candidates", []))

    out = (args.gen_root / args.variant).resolve()
    gen_dir = out / "gen"
    out.mkdir(parents=True, exist_ok=True)
    # Stale split files from a run with a different function count would
    # otherwise be globbed into the build.
    if gen_dir.exists():
        shutil.rmtree(gen_dir)

    symbols = identify_symbols(tool, xbe, out, args.title_dir / "xdk_symbols_extra.json")
    run_step("parse", xboxrecomp, ["tools.xbe_parser", xbe, "--json", out / "analysis.json", "--quiet"])
    seeds = seed_files(args.title_dir)
    seed_args = [arg for seed in seeds for arg in ("--seed-functions", seed)]
    print(f"    seeds: {len(seeds)} file(s)")
    run_step("disasm", xboxrecomp, ["tools.disasm", xbe, "--analysis-json", out / "analysis.json",
                                    "--output", out / "disasm", "--force"] + seed_args)
    run_step("func_id", xboxrecomp, ["tools.func_id", xbe,
                                     "--functions", out / "disasm" / "functions.json",
                                     "--strings", out / "disasm" / "strings.json",
                                     "--xrefs", out / "disasm" / "xrefs.json",
                                     "--output", out / "func-id"])
    run_step("abi", xboxrecomp, ["tools.abi_analysis", xbe, "--disasm-dir", out / "disasm",
                                 "--func-id-dir", out / "func-id", "--output-dir", out / "abi"])

    hle_overrides = write_hle_overrides(symbols, out / "disasm" / "functions.json", out,
                                        args.title_dir / "hle_libraries.json")
    overrides = collect_overrides(out, hle_overrides, [pathlib.Path(d) for d in override_dirs], root)
    run_step("lift", xboxrecomp, ["tools.recomp", xbe, "--all", "--split", args.split,
                                  "--disasm-dir", out / "disasm", "--func-id-dir", out / "func-id",
                                  "--abi-dir", out / "abi", "--gen-dir", gen_dir,
                                  "--output-dir", out / "recomp-summary",
                                  "--exclude-manual", overrides])
    # The overrides are compiled with the generated code they replace.
    shutil.copyfile(hle_overrides, gen_dir / "rexbox_hle_overrides.c")

    check_checkout_clean(xboxrecomp)
    print(f"done: {gen_dir}")
    return 0


if __name__ == "__main__":
    sys.exit(run())
