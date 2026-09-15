#!/usr/bin/env python3
######################################################
# Apache License, Version 2.0
# Copyright (c) 2026 chciken/Niko
######################################################
#
# Runs the named sw/<name> workload(s) against the ArduVP binary and compares
# their filtered output to a golden reference file
# (sw/<name>/<basename of name>.golden), so <name> may be a nested path like
# serial/hello_world.
# The set of workloads is not auto-discovered: it is passed in explicitly,
# normally by the fixed list in test/CMakeLists.txt.
#
# Usage:
#   test/sw_test.py serial/hello_world adc  run the named workloads
#   test/sw_test.py --record adc          (re)generate golden files
#   test/sw_test.py --binary build/arduvp_debug --timeout 30 timer

import argparse
import difflib
import re
import sys
from pathlib import Path, PurePosixPath

import pexpect

REPO_ROOT = Path(__file__).resolve().parent.parent
SW_DIR = REPO_ROOT / "sw"
DEFAULT_BINARY = REPO_ROOT / "build" / "arduvp"
DEFAULT_TIMEOUT = 60

NOISE_SUBSTRINGS = (
    "SystemC",
    "Copyright (c)",
    "ALL RIGHTS RESERVED",
    "starting simulation until",
    "Simulation stopped by user",
    "Retired instructions:",
    "Cycle count:",
    "MIPS:",
    "simulation stopped",
)

LOG_LINE_RE = re.compile(r"^\[[^\[\]]*\]\s*(.*)$")


def filter_output(raw: str) -> list[str]:
    """Strip simulator boilerplate/timestamps, keep the workload's own
    signal: raw stdout (e.g. Serial prints) and meaningful log messages."""
    lines = []
    for line in raw.splitlines():
        stripped = line.strip()
        if not stripped:
            continue
        if any(noise in stripped for noise in NOISE_SUBSTRINGS):
            continue
        match = LOG_LINE_RE.match(stripped)
        if match:
            stripped = match.group(1)
        lines.append(stripped)
    return lines


def run_workload(binary: Path, cfg: Path, timeout: int) -> tuple[list[str], int]:
    child = pexpect.spawn(str(binary), ["-f", str(cfg), "-c", "system.loglvl=error"], cwd=str(REPO_ROOT),
                           timeout=timeout, encoding="utf-8", echo=False)
    try:
        child.expect(pexpect.EOF)
    except pexpect.TIMEOUT:
        child.close(force=True)
        raise
    child.close()
    return filter_output(child.before), child.exitstatus


def run_test(name: str, binary: Path, timeout: int, record: bool) -> bool:
    sw_dir = SW_DIR / name
    stem = PurePosixPath(name).name
    cfg = sw_dir / f"{stem}.cfg"
    golden = sw_dir / f"{stem}.golden"

    if not cfg.is_file():
        print(f"[FAIL] {name}: no such config {cfg}")
        return False

    try:
        actual, exitstatus = run_workload(binary, cfg, timeout)
    except pexpect.TIMEOUT:
        print(f"[FAIL] {name}: timed out after {timeout}s")
        return False

    if exitstatus != 0:
        print(f"[FAIL] {name}: exited with status {exitstatus}")
        return False

    if record:
        golden.write_text("\n".join(actual) + "\n")
        print(f"[RECORDED] {name}: wrote {golden.relative_to(REPO_ROOT)}")
        return True

    if not golden.is_file():
        print(f"[FAIL] {name}: no golden reference at {golden.relative_to(REPO_ROOT)} "
              f"(run with --record to create it)")
        return False

    expected = golden.read_text().splitlines()
    if actual != expected:
        print(f"[FAIL] {name}: output does not match {golden.relative_to(REPO_ROOT)}")
        diff = difflib.unified_diff(expected, actual, lineterm="",
                                     fromfile="expected", tofile="actual")
        for line in diff:
            print(f"    {line}")
        return False

    print(f"[PASS] {name}")
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=DEFAULT_BINARY,
                         help="path to the arduvp executable")
    parser.add_argument("--timeout", type=int, default=DEFAULT_TIMEOUT,
                         help="per-workload timeout in seconds")
    parser.add_argument("--record", action="store_true",
                         help="(re)generate golden reference files instead of comparing")
    parser.add_argument("names", nargs="+", metavar="NAME",
                         help="sw/<NAME> workload(s) to run")
    args = parser.parse_args()

    if not args.binary.is_file():
        parser.error(f"binary not found: {args.binary}")

    results = {name: run_test(name, args.binary, args.timeout, args.record)
               for name in args.names}

    failed = [name for name, ok in results.items() if not ok]
    print(f"\n{len(results) - len(failed)}/{len(results)} passed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
