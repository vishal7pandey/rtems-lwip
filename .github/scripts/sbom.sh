#!/usr/bin/env bash
set -euo pipefail

git config --global --add safe.directory "*" || true

usage() {
  echo "Usage: sbom.sh <repo-dir> <out-dir> [--before <dir>]" >&2
  exit 2
}

if (($# < 2 || $# > 4)); then
  usage
fi

repo_dir=$(realpath "$1")
out_dir=$2
before_dir=
if (($# == 4)); then
  [[ $3 == --before ]] || usage
  before_dir=$(realpath "$4")
elif (($# != 2)); then
  usage
fi

[[ -d $repo_dir ]] || { echo "Repository directory does not exist: $repo_dir" >&2; exit 2; }
if [[ -n $before_dir && ! -d $before_dir ]]; then
  echo "Baseline directory does not exist: $before_dir" >&2
  exit 2
fi

out_dir=$(realpath -m "$out_dir")
for source_dir in "$repo_dir" ${before_dir:+"$before_dir"}; do
  if [[ $source_dir == / && $out_dir == /* ]]; then
    echo "Output directory must be outside scanned source directories: $out_dir" >&2
    exit 2
  fi
  case "$out_dir/" in
    "$source_dir/"*) echo "Output directory must be outside scanned source directories: $out_dir" >&2; exit 2 ;;
  esac
done
mkdir -p "$out_dir"

scan_sbom() {
  local source_dir=$1
  local prefix=$2
  local source_name
  source_name=$(basename "$source_dir")

  syft scan "dir:$source_dir" --source-name "$source_name" \
    -o "spdx-json=$out_dir/${prefix}spdx.json" \
    -o "cyclonedx-json=$out_dir/${prefix}cyclonedx.json"
}

scan_osv() {
  local source_dir=$1
  local report=$2
  local source_name=$3
  local status
  local scanner_status
  local scan_status

  set +e
  osv-scanner scan source --recursive "$source_dir" --format json --output-file "$report" >&2
  status=$?
  set -e

  case $status in
    0|1)
      [[ -s $report ]] || {
        echo "OSV-Scanner exited $status without producing $report" >&2
        return 1
      }
      if ((status == 0)); then
        scanner_status=clean
      else
        scanner_status=findings
      fi
      ;;
    128)
      scanner_status=no_packages
      ;;
    *)
      echo "OSV-Scanner failed for $source_name with exit code $status" >&2
      return "$status"
      ;;
  esac

  scan_status=$(python3 - "$source_dir" "$report" "$scanner_status" <<'PY'
import json
import re
import subprocess
import sys
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.parse import urljoin
from urllib.request import Request, urlopen

source_dir, report_path, scanner_status = sys.argv[1:]
root = Path(source_dir)


def git_output(*args):
    result = subprocess.run(
        ["git", "-C", str(root), *args],
        capture_output=True,
        check=False,
        text=True,
    )
    if result.returncode:
        raise SystemExit(
            f"Git command failed for {root}: {' '.join(args)}: {result.stderr.strip()}"
        )
    return result.stdout


def git_submodules():
    module_urls = {}
    for modules_file in root.rglob(".gitmodules"):
        module_root = modules_file.parent
        config = subprocess.run(
            [
                "git",
                "config",
                "-f",
                str(modules_file),
                "--get-regexp",
                r"^submodule\..*\.path$",
            ],
            capture_output=True,
            check=False,
            text=True,
        )
        if config.returncode not in (0, 1):
            raise SystemExit(
                f"Could not read submodule paths in {modules_file}: {config.stderr.strip()}"
            )

        for line in config.stdout.splitlines():
            key, module_path = line.split(None, 1)
            module_name = key.removeprefix("submodule.").removesuffix(".path")
            url = subprocess.run(
                [
                    "git",
                    "config",
                    "-f",
                    str(modules_file),
                    "--get",
                    f"submodule.{module_name}.url",
                ],
                capture_output=True,
                check=False,
                text=True,
            )
            if url.returncode:
                raise SystemExit(
                    f"Could not read URL for submodule {module_name} in {modules_file}: "
                    f"{url.stderr.strip()}"
                )
            relative_path = (module_root / module_path).relative_to(root).as_posix()
            module_urls[relative_path] = url.stdout.strip()

    if not module_urls:
        return []

    status_output = git_output("submodule", "status", "--recursive")
    status_entries = {}
    for line in status_output.splitlines():
        match = re.match(r"^[ +-U]?([0-9a-f]{40,64})\s+(.*)$", line)
        if not match:
            raise SystemExit(f"Could not parse git submodule status entry: {line}")
        commit, remainder = match.groups()
        for module_path in sorted(module_urls, key=len, reverse=True):
            if remainder == module_path or remainder.startswith(module_path + " ("):
                status_entries[module_path] = commit
                break

    origin = ""
    if any(url.startswith(("./", "../")) for url in module_urls.values()):
        origin = git_output("remote", "get-url", "origin").strip()
    sources = []
    for module_path, url in module_urls.items():
        commit = status_entries.get(module_path)
        if not commit:
            raise SystemExit(f"Git submodule status did not include {module_path}")
        if url.startswith(("./", "../")):
            url = urljoin(origin + "/", url)
        sources.append(
            {
                "type": "git",
                "url": url,
                "commit": commit,
                "path": module_path,
            }
        )
    return sources


if scanner_status == "no_packages":
    document = {"results": []}
else:
    with open(report_path, encoding="utf-8") as report:
        document = json.load(report)
if not isinstance(document, dict) or not isinstance(document.get("results"), list):
    raise SystemExit(f"OSV-Scanner produced an invalid JSON report: {report_path}")

git_sources = []
for source in git_submodules():
    request = Request(
        "https://api.osv.dev/v1/query",
        data=json.dumps({"commit": source["commit"]}).encode("utf-8"),
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    try:
        with urlopen(request, timeout=30) as response:
            result = json.load(response)
    except (HTTPError, URLError, TimeoutError) as error:
        raise SystemExit(
            f"OSV commit query failed for {source['path']} ({source['commit']}): {error}"
        )
    vulnerabilities = result.get("vulns") or []
    git_sources.append({"source": source, "vulnerabilities": vulnerabilities})

document["report_type"] = "anvil.osv-scan.v1"
document["scanner_status"] = scanner_status
document["git_sources"] = git_sources
has_findings = scanner_status == "findings" or any(
    entry["vulnerabilities"] for entry in git_sources
)
if has_findings:
    scan_status = "findings"
elif scanner_status == "no_packages" and not git_sources:
    scan_status = "no_packages"
else:
    scan_status = "clean"
document["scan_status"] = scan_status

with open(report_path, "w", encoding="utf-8") as report:
    json.dump(document, report, indent=2)
    report.write("\n")
print(scan_status)
PY
)

  case $scan_status in
    findings) echo "OSV scan found vulnerabilities in $source_name" >&2 ;;
    clean) echo "OSV scan completed without known findings in $source_name" >&2 ;;
    no_packages) echo "OSV found no supported package sources or Git submodules in $source_name" >&2 ;;
  esac
}

scan_sbom "$repo_dir" ""
after_status=$(scan_osv "$repo_dir" "$out_dir/osv.json" "$(basename "$repo_dir")")

if [[ -n $before_dir ]]; then
  scan_sbom "$before_dir" "before-"
  before_status=$(scan_osv "$before_dir" "$out_dir/osv-before.json" "$(basename "$before_dir")")

  python3 - "$out_dir/osv-before.json" "$out_dir/osv.json" \
    "$before_status" "$after_status" "$out_dir/osv-delta.json" <<'PY'
import json
import sys

before_path, after_path, before_status, after_status, output_path = sys.argv[1:]


def load_findings(path):
    with open(path, encoding="utf-8") as report:
        document = json.load(report)

    findings = {}

    def add_vulnerabilities(identity, vulnerabilities, groups, identity_key=None):
        for vulnerability in vulnerabilities or []:
            identifiers = {
                value
                for value in [
                    vulnerability.get("id"),
                    *(vulnerability.get("aliases") or []),
                ]
                if value
            }
            for group in groups or []:
                group_ids = set(group.get("ids") or [])
                if identifiers & group_ids:
                    identifiers.update(group_ids)
            if not identifiers:
                continue

            canonical_ids = sorted(identifiers)
            primary_id = vulnerability.get("id") or canonical_ids[0]
            key = (identity_key or json.dumps(identity, sort_keys=True), primary_id)
            findings[key] = {
                "package": identity,
                "vulnerability_ids": canonical_ids,
            }

    for result in document.get("results") or []:
        for package_result in result.get("packages") or []:
            package = package_result.get("package", {})
            package_identity = {
                "ecosystem": package.get("ecosystem"),
                "name": package.get("name"),
                "version": package.get("version"),
                "purl": package.get("purl"),
            }
            groups = package_result.get("groups") or []
            for vulnerability in package_result.get("vulnerabilities") or []:
                add_vulnerabilities(
                    package_identity,
                    [vulnerability],
                    [*groups, *(vulnerability.get("groups") or [])],
                )

    for entry in document.get("git_sources") or []:
        source = entry.get("source") or {}
        identity = {
            "ecosystem": "GIT",
            "name": source.get("url"),
            "version": source.get("commit"),
            "purl": None,
            "path": source.get("path"),
        }
        repository_identity = json.dumps(
            {"ecosystem": "GIT", "name": source.get("url"), "path": source.get("path")},
            sort_keys=True,
        )
        add_vulnerabilities(
            identity,
            entry.get("vulnerabilities"),
            [],
            identity_key=repository_identity,
        )
    return findings


before = load_findings(before_path)
after = load_findings(after_path)
introduced = [after[key] for key in sorted(after.keys() - before.keys())]
resolved = [before[key] for key in sorted(before.keys() - after.keys())]
incomplete = "no_packages" in (before_status, after_status)

delta = {
    "schema": "anvil.osv-delta.v1",
    "comparison_status": "incomplete_no_packages" if incomplete else "complete",
    "before_scan_status": before_status,
    "after_scan_status": after_status,
    "introduced": introduced,
    "resolved": resolved,
    "counts": {"introduced": len(introduced), "resolved": len(resolved)},
}
with open(output_path, "w", encoding="utf-8") as report:
    json.dump(delta, report, indent=2)
    report.write("\n")
PY
fi
