#!/usr/bin/env vpython3
# pylint: disable=line-too-long
"""This script runs `npm audit' and `cargo audit' on relevant paths in the
repo."""

# Copyright (c) 2020 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import json
import os
import re
import subprocess
import shutil
import sys
import tempfile
import urllib.error
import urllib.request


def get_remote_audit_config(
    url="https://raw.githubusercontent.com/brave/audit-config/main/config.json",
):
    return json.loads(urllib.request.urlopen(url).read().decode("utf-8"))


REMOTE_AUDIT_CONFIG = get_remote_audit_config()
IGNORED_CARGO_ADVISORIES = [
    e["advisory"] for e in REMOTE_AUDIT_CONFIG["ignore"]["cargo"]
]
IGNORED_NPM_ADVISORIES = [
    e["advisory"] for e in REMOTE_AUDIT_CONFIG["ignore"]["npm"]
]

# Use all (sub)paths except these for npm audit.
NPM_EXCLUDE_PATHS = [
    'build',
    os.path.join('node_modules'),
    os.path.join('tools', 'crates'),
]

# Matches `github:owner/repo#revision` and
# `git+https://github.com/owner/repo.git#revision` dependency specifiers.
GIT_DEP_PATTERN = re.compile(
    r'^(?:github:|git\+https://github\.com/)'
    r'([^/]+)/([^#/]+?)(?:\.git)?#(.+)$'
)

# Only check Cargo.lock for these paths.
CARGO_INCLUDE_PATHS = [
    os.path.join('third_party', 'rust', 'chromium_crates_io'),
    os.path.join('tools', 'crates'),
    os.path.join('components', 'skus', 'browser', 'rs', 'wasm'),
]


def main():
    """Audit a specified path, or the whole project."""

    if len(IGNORED_NPM_ADVISORIES) > 0:
        print(
            f"Ignoring NPM advisories "
            f"{', '.join(map(str, IGNORED_NPM_ADVISORIES))}"
        )
    if len(IGNORED_CARGO_ADVISORIES) > 0:
        print(
            f"Ignoring Cargo advisories "
            f"{', '.join(map(str, IGNORED_CARGO_ADVISORIES))}"
        )

    args = parse_args()
    args.source_root = os.path.abspath(args.source_root)
    errors = 0

    if args.input_dir:
        return audit_path(os.path.abspath(args.input_dir), args)

    for path in [
        os.path.dirname(os.path.dirname(args.source_root)),
        args.source_root,
    ]:
        errors += audit_path(path, args)

    for dir_path, dirs, _ in os.walk(args.source_root):
        for dir_name in dirs:
            full_path = os.path.join(dir_path, dir_name)
            errors += audit_path(full_path, args)

    errors += audit_git_dependencies(args)

    for p in CARGO_INCLUDE_PATHS:
        print(f'Auditing (cargo) {p}')
        errors += cargo_audit_deps(os.path.join(args.source_root, p), args)

    if args.output:
        with open(args.output, 'w', encoding='utf-8') as f:
            json.dump(errors, f)

    return errors > 0


def audit_path(path, args):
    """Audit the specified path (relative, or absolute)."""
    errors = 0

    full_path = os.path.join(os.path.abspath(path), "")
    if (
        os.path.isfile(os.path.join(path, 'package.json'))
        and os.path.isfile(os.path.join(path, 'package-lock.json'))
        and not any(
            full_path.startswith(os.path.join(args.source_root, p, ""))
            for p in NPM_EXCLUDE_PATHS
        )
    ):
        print(f'Auditing (npm) {path}')
        errors += npm_audit_deps(path, args)

    if (
        os.path.isfile(os.path.join(path, 'package.json'))
        and os.path.isfile(os.path.join(path, 'pnpm-lock.yaml'))
        and not any(
            full_path.startswith(os.path.join(args.source_root, p, ""))
            for p in NPM_EXCLUDE_PATHS
        )
    ):
        print(f'Auditing (pnpm) {path}')
        errors += pnpm_audit_deps(path, args)

    return errors


def audit_git_dependencies(args):
    """Audit every git-hosted dependency against its own lockfile.

    A git dependency's dev dependencies are installed by its `prepare` script
    and appear in neither brave-core's lockfile nor the packed copy under
    node_modules, so no other pass in this script can see them. The same is
    true of git dependencies nested inside those manifests, so the whole graph
    is walked.
    """
    errors = 0
    manifest_path = os.path.join(args.source_root, 'package.json')
    with open(manifest_path, encoding='utf-8') as f:
        manifest = json.load(f)

    visited = set()
    queue = git_dependencies(manifest)
    while queue:
        name, owner, repo, revision = queue.pop(0)
        if (owner, repo, revision) in visited:
            continue
        visited.add((owner, repo, revision))
        dep_errors, nested = audit_git_dependency(
            name, owner, repo, revision, args
        )
        errors += dep_errors
        queue += nested

    return errors


def git_dependencies(manifest):
    """List the (name, owner, repo, revision) git deps a manifest declares."""

    deps = {}
    for section in ('dependencies', 'devDependencies', 'optionalDependencies'):
        deps.update(manifest.get(section) or {})

    found = []
    for name, specifier in sorted(deps.items()):
        match = GIT_DEP_PATTERN.match(specifier)
        if match:
            found.append((name, *match.groups()))
    return found


def audit_git_dependency(name, owner, repo, revision, args):
    """Audit a single git-hosted dependency at its pinned revision.

    Returns the error count and the git dependencies of its manifest.
    """

    base_url = f'https://raw.githubusercontent.com/{owner}/{repo}/{revision}'
    manifest = fetch_file(f'{base_url}/package.json')
    if manifest is None:
        print(f'Result: {name} has no package.json at {revision}')
        return 1, []

    # Let corepack use the ambient package manager rather than downloading the
    # one the dependency pins; only the lockfile is being read.
    manifest = json.loads(manifest)
    manifest.pop('packageManager', None)
    nested = git_dependencies(manifest)

    for lockfile, audit in (
        ('pnpm-lock.yaml', pnpm_audit_deps),
        ('package-lock.json', npm_audit_deps),
    ):
        lock = fetch_file(f'{base_url}/{lockfile}')
        if lock is None:
            continue
        with tempfile.TemporaryDirectory() as path:
            with open(
                os.path.join(path, 'package.json'), 'w', encoding='utf-8'
            ) as f:
                json.dump(manifest, f)
            with open(os.path.join(path, lockfile), 'wb') as f:
                f.write(lock)
            print(f'Auditing (git) {name} at {revision}')
            return audit(path, args), nested

    print(f'Skipping {name}: no lockfile at {revision}')
    return 0, nested


def fetch_file(url):
    """Fetch a URL, returning None if it does not exist."""

    try:
        return urllib.request.urlopen(url).read()
    except urllib.error.HTTPError as e:
        if e.code == 404:
            return None
        raise


def npm_audit_deps(path, args):
    """Run `npm audit' in the specified path."""

    npm_cmd = 'npm'
    if sys.platform.startswith('win'):
        npm_cmd = 'npm.cmd'

    npm_args = [npm_cmd, 'audit', '--json']
    if not args.audit_dev_deps:
        # Don't support npm audit --production until dev dependencies are
        # correctly identified in package.json
        print('npm audit --production not supported; auditing dev dependencies')
    with subprocess.Popen(
        npm_args, stdout=subprocess.PIPE, cwd=path
    ) as audit_process:
        output, _ = audit_process.communicate()

    try:
        # results from audit
        result = json.loads(output.decode('UTF-8'))
        # npm7 uses a different format from earlier versions
        assert 'vulnerabilities' in result or 'advisories' in result
    except (ValueError, AssertionError):
        # This can happen in the case of an NPM network error
        print('Audit failed to return valid json')
        return 1

    resolutions = extract_resolutions(result)

    if len(resolutions) > 0:
        print('Result: Audit failed due to vulnerabilities')
        print(json.dumps(resolutions, indent=2))
        return 1

    print('Result: Audit finished, no vulnerabilities found')
    return 0


def pnpm_audit_deps(path, args):
    """Run `pnpm audit' in the specified path."""

    pnpm_path = shutil.which('pnpm')
    if not pnpm_path:
        print('pnpm not found')
        return 1

    pnpm_args = [pnpm_path, 'audit', '--json']
    if not args.audit_dev_deps:
        # Don't support pnpm audit --prod until dev dependencies are
        # correctly identified in package.json
        print('pnpm audit --prod not supported; auditing dev dependencies')
    with subprocess.Popen(
        pnpm_args, stdout=subprocess.PIPE, cwd=path
    ) as audit_process:
        output, _ = audit_process.communicate()

    try:
        # results from audit
        result = json.loads(output.decode('UTF-8'))
        assert 'vulnerabilities' in result or 'advisories' in result
    except (ValueError, AssertionError):
        # This can happen in the case of a network error
        print('Audit failed to return valid json')
        return 1

    resolutions = extract_resolutions(result)

    if len(resolutions) > 0:
        print('Result: Audit failed due to vulnerabilities')
        print(json.dumps(resolutions, indent=2))
        return 1

    print('Result: Audit finished, no vulnerabilities found')
    return 0


def cargo_audit_deps(path, args):
    """Run `cargo audit' in the specified path."""
    cargo_args = []
    cargo_args.append(args.cargo_audit_exe)
    cargo_args.append("audit")
    cargo_args.append("--file")
    cargo_args.append(os.path.join(path, "Cargo.lock"))
    for advisory in IGNORED_CARGO_ADVISORIES:
        cargo_args.append("--ignore")
        cargo_args.append(advisory)

    return subprocess.call(cargo_args)


def extract_resolutions(result):
    """Extract resolutions from advisories present in the result."""

    resolutions = []
    # npm 7+
    if 'vulnerabilities' in result:
        advisories = result['vulnerabilities']
        if len(advisories) == 0:
            return resolutions
        for _, v in advisories.items():
            via = v['via']
            for item in via:
                if (
                    isinstance(item, dict)
                    and item['url'] not in IGNORED_NPM_ADVISORIES
                ):
                    resolutions.append(item['url'])
    # npm 6 and earlier
    if 'advisories' in result:
        advisories = result['advisories']
        if len(advisories) == 0:
            return resolutions
        for _, v in advisories.items():
            url = v['url']
            if url not in IGNORED_NPM_ADVISORIES:
                resolutions.append(url)
    return resolutions


def parse_args():
    """Parse command line arguments."""

    parser = argparse.ArgumentParser(description='Audit brave-core npm deps')
    parser.add_argument('input_dir', nargs='?', help='Directory to check')
    parser.add_argument(
        '--source_root',
        required=True,
        help='Full path of the src/brave directory',
    )
    parser.add_argument('--cargo_audit_exe', required=True)
    parser.add_argument(
        '--audit_dev_deps', action='store_true', help='Audit dev dependencies'
    )
    parser.add_argument('--output', help='Output file')
    return parser.parse_args()


if __name__ == '__main__':
    sys.exit(main())
