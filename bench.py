#!/usr/bin/env python3
"""Bench harness: run the firmware self-test on a target and print a JSON verdict.

    python bench.py test --target host
    python bench.py test --target esp32 --port COM5      (needs PlatformIO and pyserial)

Both targets emit the same line protocol (see firmware/lib/sensor/selftest.h),
parsed by one function. Exit code: 0 all tests passed, 1 a test failed,
2 harness error (build, flash, timeout, malformed output).
"""

import argparse
import json
import shutil
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent
FIRMWARE = ROOT / "firmware"
BUILD = ROOT / "build"
SERIAL_BAUD = 115200
RUN_TIMEOUT_S = 30


class BenchError(Exception):
    pass


def parse_run(lines):
    """Consume protocol lines from BEGIN to DONE and return the test results.

    Lines before BEGIN and lines that are not protocol lines (boot noise) are ignored.
    Raises BenchError if the run is incomplete or the summary disagrees with the tests.
    """
    tests = None
    summary = None
    for raw in lines:
        line = raw.strip()
        if line == "BEGIN":
            tests, summary = [], None
        elif tests is None:
            continue
        elif line.startswith("TEST "):
            parts = line.split(" ", 3)
            if len(parts) < 3 or parts[2] not in ("PASS", "FAIL"):
                raise BenchError(f"malformed TEST line: {line!r}")
            detail = parts[3] if len(parts) == 4 else ""
            tests.append({"name": parts[1], "status": parts[2], "detail": detail})
        elif line.startswith("SUMMARY "):
            fields = dict(item.split("=", 1) for item in line.split()[1:])
            summary = (int(fields["pass"]), int(fields["fail"]))
        elif line == "DONE":
            if summary is None:
                raise BenchError("DONE without SUMMARY")
            passed = sum(t["status"] == "PASS" for t in tests)
            failed = sum(t["status"] == "FAIL" for t in tests)
            if (passed, failed) != summary:
                raise BenchError(f"SUMMARY {summary} disagrees with tests ({passed}, {failed})")
            return tests
    raise BenchError("output ended before DONE")


def run_host():
    BUILD.mkdir(exist_ok=True)
    binary = BUILD / "test_host"
    lib = FIRMWARE / "lib" / "sensor"
    subprocess.run(
        ["gcc", "-Wall", "-Wextra", "-Werror", f"-I{lib}", "-o", str(binary),
         str(FIRMWARE / "host" / "test_host.c"), str(lib / "sensor.c"), str(lib / "selftest.c")],
        check=True,
    )
    out = subprocess.run([str(binary)], check=True, capture_output=True, text=True, timeout=RUN_TIMEOUT_S)
    return parse_run(out.stdout.splitlines())


def serial_lines(port, deadline):
    import serial  # imported here: only the esp32 target needs pyserial

    with serial.Serial(port, SERIAL_BAUD, timeout=1) as conn:
        while time.monotonic() < deadline:
            line = conn.readline().decode("utf-8", errors="replace")
            if line:
                yield line
    raise BenchError(f"no complete run from {port} within {RUN_TIMEOUT_S}s")


def run_esp32(port):
    pio = shutil.which("pio")
    if pio is None:
        raise FileNotFoundError("PlatformIO executable 'pio' was not found on PATH")
    subprocess.run(
        [pio, "run", "-d", str(FIRMWARE), "-t", "upload", "--upload-port", port],
        check=True,
    )
    return parse_run(serial_lines(port, time.monotonic() + RUN_TIMEOUT_S))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    test = sub.add_parser("test", help="run the self-test on a target")
    test.add_argument("--target", choices=["host", "esp32"], required=True)
    test.add_argument("--port", help="serial port, required for esp32")
    args = parser.parse_args()

    if args.target == "esp32" and not args.port:
        parser.error("--port is required for --target esp32")
    if args.target == "host" and args.port:
        parser.error("--port applies only to --target esp32")

    try:
        tests = run_host() if args.target == "host" else run_esp32(args.port)
    except (BenchError, subprocess.CalledProcessError, subprocess.TimeoutExpired,
            FileNotFoundError, ImportError, OSError) as exc:
        print(json.dumps({"target": args.target, "verdict": "ERROR", "error": f"{type(exc).__name__}: {exc}"}))
        return 2

    failed = [t for t in tests if t["status"] == "FAIL"]
    print(json.dumps({
        "target": args.target,
        "verdict": "FAIL" if failed else "PASS",
        "passed": len(tests) - len(failed),
        "failed": len(failed),
        "tests": tests,
    }, indent=2))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
