#!/usr/bin/env python3
"""Validate release identity and export the committed, buildable source tree."""

import argparse
import json
import re
import subprocess
from pathlib import Path


def identity(upstream, tag, run_number, commit):
    if not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+", upstream):
        raise ValueError("configure.ac must contain a numeric x.y.z version")
    if tag:
        match = re.fullmatch(r"v([0-9]+\.[0-9]+\.[0-9]+)-([1-9][0-9]*)", tag)
        if not match or match[1] != upstream:
            raise ValueError("release tag must be v<configure.ac version>-<positive revision>")
        revision = match[2]
    else:
        if not re.fullmatch(r"[1-9][0-9]*", run_number):
            raise ValueError("run number must be a positive integer")
        revision = f"0.ci.{run_number}.g{commit[:12]}"
    return {"version": upstream, "revision": revision, "tag": tag,
            "commit": commit, "package_version": f"{upstream}-{revision}",
            "source": f"dante-{upstream}-{revision}.tar.gz"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository", type=Path, default=Path.cwd())
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--tag", default="")
    parser.add_argument("--run-number", default="1")
    args = parser.parse_args()
    root = args.repository.resolve()

    def git(*arguments):
        return subprocess.check_output(["git", "-C", str(root), *arguments])

    commit = git("rev-parse", "HEAD").decode().strip()
    config = git("show", "HEAD:dante/configure.ac").decode()
    upstream = re.search(r"^version=(\S+)$", config, re.M)[1]
    data = identity(upstream, args.tag, args.run_number, commit)
    if args.tag and git("rev-parse", f"refs/tags/{args.tag}^{{commit}}").decode().strip() != commit:
        raise ValueError("release tag does not identify the checked-out commit")
    args.output.mkdir(parents=True, exist_ok=True)
    archive = args.output / data["source"]
    # Only committed files; no local findings, build outputs or working-tree files.
    subprocess.run(["git", "-C", str(root), "archive", "--format=tar.gz",
                    f"--prefix=dante-{data['version']}/", f"--output={archive.resolve()}",
                    "HEAD:dante"], check=True)
    (args.output / "build-metadata.json").write_text(json.dumps(data, indent=2) + "\n")
    print(json.dumps(data, indent=2))


if __name__ == "__main__":
    main()
