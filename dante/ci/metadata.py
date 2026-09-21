#!/usr/bin/env python3
"""Validate release identity and export the committed, buildable source tree."""

import argparse
import json
import re
import subprocess
import tarfile
import tempfile
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


DEBIAN_TARGETS = {"ubuntu22.04", "debian12"}


def package_version_for_target(data, target):
    if target not in DEBIAN_TARGETS:
        raise ValueError(f"unknown Debian target: {target}")
    return f"{data['package_version']}+{target}"


def select_targets(targets, full_matrix):
    if full_matrix:
        return targets
    selected = [target for target in targets if target.get("format") == "deb"]
    expected = {(target, arch) for target in DEBIAN_TARGETS for arch in ("amd64", "arm64")}
    actual = {(target.get("target"), target.get("arch")) for target in selected}
    if actual != expected:
        raise ValueError(f"Debian PR matrix mismatch: expected={expected}, actual={actual}")
    return selected


def export_source(root, archive, version, commit):
    """Export upstream and Debian packaging from the same committed tree."""
    prefix = f"dante-{version}"
    with tempfile.TemporaryDirectory(prefix="dante-source-") as directory:
        temporary = Path(directory)
        upstream_tar = temporary / "upstream.tar"
        debian_tar = temporary / "debian.tar"
        subprocess.run(
            ["git", "-C", str(root), "archive", "--format=tar",
             f"--output={upstream_tar}", f"{commit}:dante"],
            check=True,
        )
        subprocess.run(
            ["git", "-C", str(root), "archive", "--format=tar",
             f"--output={debian_tar}", f"{commit}:debian"],
            check=True,
        )
        with tarfile.open(archive, "w:gz") as output:
            for source_path, nested in ((upstream_tar, ""), (debian_tar, "debian")):
                with tarfile.open(source_path, "r:") as source:
                    for member in source.getmembers():
                        member.name = "/".join(part for part in (prefix, nested, member.name) if part)
                        payload = source.extractfile(member) if member.isfile() else None
                        output.addfile(member, payload)


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
    config = git("show", f"{commit}:dante/configure.ac").decode()
    upstream = re.search(r"^version=(\S+)$", config, re.M)[1]
    data = identity(upstream, args.tag, args.run_number, commit)
    if args.tag and git("rev-parse", f"refs/tags/{args.tag}^{{commit}}").decode().strip() != commit:
        raise ValueError("release tag does not identify the checked-out commit")
    args.output.mkdir(parents=True, exist_ok=True)
    archive = args.output / data["source"]
    # Only committed files; both trees come from the exact commit validated above.
    export_source(root, archive.resolve(), data["version"], commit)
    (args.output / "build-metadata.json").write_text(json.dumps(data, indent=2) + "\n")
    print(json.dumps(data, indent=2))


if __name__ == "__main__":
    main()
