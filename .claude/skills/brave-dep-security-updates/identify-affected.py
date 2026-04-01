#!/usr/bin/env python3
# Identify which package/repo is affected by a GHSA or RUSTSEC advisory.
# Usage: python3 identify-affected.py <GHSA-xxxx-xxxx-xxxx | RUSTSEC-YYYY-NNNN>
#
# Set env vars to override default repo paths:
#   BRAVE_CORE   — path to brave-core (defaults to cwd git root)
#   WDP_DIR      — path to web-discovery-project
#   LEO_DIR      — path to leo

import json
import os
import subprocess
import sys


def run(cmd, cwd=None):
    result = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
    return result.stdout.strip(), result.returncode


def git_root(path=None):
    out, rc = run(["git", "rev-parse", "--show-toplevel"], cwd=path or os.getcwd())
    return out if rc == 0 else ""


def check_npm_location(label, directory, pkg_name):
    if not os.path.isdir(directory):
        print(f"  [{label}] Directory not found: {directory}")
        return False
    print(f"\n--- {label} ({directory}) ---")
    out, _ = run(["npm", "ls", pkg_name, "--depth=10"], cwd=directory)
    if out:
        print(out[:1000])
    out2, _ = run(["npm", "why", pkg_name], cwd=directory)
    if out2:
        print(f"\nnpm why {pkg_name}:")
        print(out2[:1000])
    return bool(out or out2)


def check_rust(advisory, brave_core):
    crates_dir = os.path.join(brave_core, "tools", "crates")
    if not os.path.isdir(crates_dir):
        print(f"tools/crates not found in {brave_core}")
        return
    print(f"\n--- brave-core rust ({crates_dir}) ---")
    out, _ = run(["cargo", "audit", "--json"], cwd=crates_dir)
    if not out:
        print("cargo audit produced no output (may need cargo-audit installed)")
        return
    try:
        data = json.loads(out)
        vulns = data.get("vulnerabilities", {}).get("list", [])
        for v in vulns:
            adv = v.get("advisory", {})
            if adv.get("id") == advisory:
                pkg = v.get("package", {})
                print(f"FOUND: {pkg.get('name')} {pkg.get('version')}")
                print(f"  Title: {adv.get('title')}")
                print(f"  CVSS:  {adv.get('cvss')}")
    except json.JSONDecodeError:
        # Fallback: grep text output
        out_text, _ = run(["cargo", "audit"], cwd=crates_dir)
        lines = [l for l in out_text.splitlines() if advisory in l or "warning" in l.lower()]
        print("\n".join(lines[:20]) if lines else "(advisory not found in cargo audit output)")


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 identify-affected.py <GHSA-xxxx-xxxx-xxxx | RUSTSEC-YYYY-NNNN>")
        sys.exit(1)

    advisory = sys.argv[1]
    brave_core = os.environ.get("BRAVE_CORE") or git_root()
    wdp_dir = os.environ.get("WDP_DIR", "")
    leo_dir = os.environ.get("LEO_DIR", "")

    print(f"Advisory: {advisory}")
    print(f"brave-core: {brave_core or '(not found)'}")
    if wdp_dir:
        print(f"WDP:        {wdp_dir}")
    if leo_dir:
        print(f"Leo:        {leo_dir}")

    if advisory.startswith("RUSTSEC-"):
        check_rust(advisory, brave_core)
        return

    # GHSA — npm advisory
    # Try to resolve package name from npm audit
    found_any = False
    for label, directory in [
        ("brave-core", brave_core),
        ("web-discovery-project", wdp_dir),
        ("leo", leo_dir),
    ]:
        if not directory:
            continue
        # Check npm audit for this GHSA
        print(f"\n=== Checking {label} ===")
        audit_out, _ = run(["npm", "audit", "--json"], cwd=directory)
        if audit_out:
            try:
                audit = json.loads(audit_out)
                vulns = audit.get("vulnerabilities", {})
                for pkg_name, info in vulns.items():
                    via = info.get("via", [])
                    for v in via:
                        if isinstance(v, dict) and advisory in v.get("url", ""):
                            print(f"  FOUND in {pkg_name}")
                            print(f"    Severity: {info.get('severity')}")
                            print(f"    Fix available: {info.get('fixAvailable')}")
                            found_any = True
            except json.JSONDecodeError:
                pass

        if not found_any:
            # Fallback: npm audit text
            audit_text, _ = run(["npm", "audit"], cwd=directory)
            if advisory in audit_text:
                lines = [l for l in audit_text.splitlines() if advisory in l or "npm audit fix" in l]
                print("\n".join(lines[:10]))
                found_any = True
            else:
                print("  (not found)")

    if not found_any:
        print(f"\n{advisory} not found in any checked repository.")
        print("Set WDP_DIR and/or LEO_DIR env vars if those repos weren't checked.")


if __name__ == "__main__":
    main()
