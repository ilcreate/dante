#!/usr/bin/env python3
"""Measure identical real-logger events; observations only, no speed threshold.

Half formatted diagnostics, half structured TCP session closes. The C driver
times only the logging loop (real clock, allocator, serializer, write); startup
and Python are excluded. Output is /dev/null by default, or an append-only file
with --file. No fsync: this measures logger and buffered kernel writes, not
durable storage or network throughput. Small preflight files prove identical
event messages/counts and report output size. Paired runs alternate order.
"""
import argparse
import json
import os
from pathlib import Path
import platform
import statistics
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--events", type=int, default=100000)
    parser.add_argument("--samples", type=int, default=7)
    parser.add_argument("--file", type=Path, help="append benchmark output to this file")
    args = parser.parse_args()
    if args.events < 2 or args.samples < 1:
        parser.error("events must be at least 2 and samples at least 1")
    binary = str(args.binary.resolve())

    def run(fmt, count, output):
        completed = subprocess.run([binary, fmt, str(count)], stdout=output,
                                   stderr=subprocess.PIPE, check=True, timeout=120)
        sample = json.loads(completed.stderr)
        assert sample["events"] == count and sample["format"] == fmt
        return sample

    sizes, messages = {}, {}
    with tempfile.TemporaryDirectory(prefix="dante-benchmark-", dir="/tmp") as tmp:
        for fmt in ("raw", "json"):
            path = Path(tmp) / fmt
            with path.open("ab") as output:
                run(fmt, 20, output)
            data = path.read_bytes()
            lines = data.decode().splitlines()
            assert len(lines) == 20 and data.endswith(b"\n")
            sizes[fmt] = len(data) / 20
            if fmt == "json":
                events = [json.loads(line) for line in lines]
                assert all(e["schema_version"] == 1 and not e["truncated"] for e in events)
                assert sum(e["event"] == "session_close" for e in events) == 10
                messages[fmt] = [e["message"] for e in events]
            else:
                messages[fmt] = [line.split(": info: ", 1)[1] for line in lines]
        assert messages["raw"] == messages["json"]
    samples = {fmt: [] for fmt in ("raw", "json")}
    with open(args.file or os.devnull, "ab") as output:
        for fmt in samples:
            run(fmt, min(args.events, 10000), output)  # warmup, not reported
        for index in range(args.samples):
            for fmt in (("raw", "json") if index % 2 == 0 else ("json", "raw")):
                samples[fmt].append(run(fmt, args.events, output))
    medians = {fmt: {key: statistics.median(s[key] for s in entries)
                     for key in ("wall_seconds", "cpu_seconds")}
               for fmt, entries in samples.items()}
    result = {
        "platform": platform.platform(), "machine": platform.machine(),
        "binary": binary, "destination": str(args.file or os.devnull),
        "events_per_sample": args.events, "samples_per_format": args.samples,
        "workload": "50% formatted diagnostics; 50% structured TCP session_close",
        "preflight_bytes_per_event": sizes, "median": medians,
        "json_over_raw_ratio": {key: medians["json"][key] / medians["raw"][key]
                                for key in medians["raw"]},
        "samples": samples,
    }
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
