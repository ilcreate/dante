#!/usr/bin/env python3
"""Assemble a complete release; never overwrite a published release."""

import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path


def verify_assets(directory):
    data = json.loads((directory / "build-metadata.json").read_text())
    version, revision = data["version"], data["revision"]
    if not data["tag"]:
        raise ValueError("CI preview artifacts cannot be released")
    required = {data["source"], "build-metadata.json"}
    for arch, rpm_arch in [("amd64", "x86_64"), ("arm64", "aarch64")]:
        for name in [f"dante-server_{version}-{revision}_debian12_{arch}.deb",
                     f"dante-server-{version}-{revision}.el9.{rpm_arch}.rpm",
                     f"dante-server-{version}-{revision}-macos15-{arch}.pkg"]:
            required.update([name, name + ".build-info.json"])
    actual = {p.name for p in directory.iterdir() if p.is_file()} - {"SHA256SUMS"}
    if actual != required:
        raise ValueError(f"release assets mismatch: missing={required - actual}, extra={actual - required}")
    for name in required:
        if (directory / name).stat().st_size == 0:
            raise ValueError(f"empty artifact: {name}")
        if name.endswith(".build-info.json"):
            info = json.loads((directory / name).read_text())
            for key in ["commit", "tag", "package_version"]:
                if info[key] != data[key]:
                    raise ValueError(f"{name}: inconsistent {key}")
    return data, sorted(required)


def main():
    directory = Path(sys.argv[1]).resolve()
    data, names = verify_assets(directory)
    if data["commit"] != os.environ["GITHUB_SHA"] or data["tag"] != os.environ["GITHUB_REF_NAME"]:
        raise ValueError("artifacts do not match release workflow commit/tag")
    checksums = []
    for name in names:
        with (directory / name).open("rb") as file:
            digest = hashlib.file_digest(file, "sha256").hexdigest()
        checksums.append(f"{digest}  {name}\n")
    (directory / "SHA256SUMS").write_text("".join(checksums))
    repo = os.environ["GITHUB_REPOSITORY"]
    tag = data["tag"]
    # List is used instead of treating any `gh release view` failure as a 404.
    raw = subprocess.check_output(["gh", "api", "--paginate", f"repos/{repo}/releases", "--jq", ".[] | [.tag_name, .draft] | @json"], text=True)
    releases = [dict(zip(("tag_name", "draft"), json.loads(line))) for line in raw.splitlines() if line]
    matching = [release for release in releases if release["tag_name"] == tag]
    if matching and not matching[0]["draft"]:
        raise RuntimeError("refusing to overwrite a published release; use a new revision tag")
    if not matching:
        subprocess.run(["gh", "release", "create", tag, "--repo", repo, "--verify-tag", "--draft",
                        "--title", f"Dante {data['package_version']}", "--generate-notes"], check=True)
    subprocess.run(["gh", "release", "upload", tag, "--repo", repo, "--clobber",
                    *[str(directory / name) for name in names + ["SHA256SUMS"]]], check=True)
    print(f"Draft {tag} is ready. Review the artifacts and publish it from GitHub Releases.")


if __name__ == "__main__":
    main()
