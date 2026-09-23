# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Extracts swiftc arguments for GN Swift targets out of a ninja build dir.

GN compiles Swift through the `swift` ninja rule, which runs
//build/toolchain/apple/swiftc.py rather than swiftc directly. That wrapper is
the only thing that knows the final swiftc command line, so the arguments are
recovered by:

  1. Scanning the generated `obj/**/*.ninja` files for `swift` build edges to
     find Swift targets, their sources and their ninja variables.
  2. Expanding the `rule swift` command template from `toolchain.ninja` with
     those variables to rebuild the swiftc.py invocation.
  3. Running swiftc.py as a subprocess with `--swift-toolchain-path` pointed at
     a fake toolchain whose `swiftc` records its arguments and exits non-zero,
     so it assembles the command line without compiling anything.
  4. Rewriting the result into something an index/language server can consume
     (dropping code generation flags, expanding the `@file.SwiftFileList`
     response file).

Can be run standalone for debugging:

    python3 gn_swift_args.py --out-dir out/ios_sim path/to/File.swift
"""

import argparse
import json
import os
import re
import shlex
import subprocess
import sys

# Substituted for the temporary directory swiftc.py uses for the module cache,
# precompiled headers and index store, so the paths outlive the extraction.
LSP_CACHE_DIR_NAME = 'swift_lsp_cache'

# Caches that are safe to share between targets, mapped to their shared
# location inside the cache directory. swiftc.py points these at a per-target
# derived data directory, which is right for a build (where an incremental
# rebuild of one target should not disturb another) but wasteful here: entries
# are keyed by a hash of the compilation settings, so targets built with the
# same flags produce byte-identical entries and can reuse each other's work,
# while targets with differing flags coexist under different hashes.
#
# `-pch-output-dir` is deliberately not shared. Bridging header PCHs are named
# `<header basename>-<hash>.pch`, and that hash does not distinguish two targets
# with different `bridge_header.h` files, so sharing makes them collide and
# emitting the PCH fails.
_SHARED_CACHE_FLAGS = {
    '-module-cache-path': 'SharedModuleCache.noindex',
}

# swiftc arguments that produce build outputs. They are useless for indexing,
# so they are stripped to keep the argument list to what describes how the
# module is parsed and typechecked. This is a correctness/tidiness measure for
# sourcekit-lsp, not the build-isolation guarantee: `--target-out-dir` and
# `--header-path` are redirected into the cache directory (see
# `_extract_swiftc_command`), so nothing swiftc.py writes can reach the real
# build directory regardless of which of these survive.
_DROPPED_FLAGS = frozenset([
    '-c',
    '-emit-const-values',
    '-emit-dependencies',
    '-emit-module',
    '-emit-objc-header',
    '-incremental',
    '-save-temps',
    '-serialize-diagnostics',
    '-whole-module-optimization',
    '-no-emit-module-separately-wmo',
    '-experimental-emit-module-separately',
    '-enable-batch-mode',
])

# Same, but the flag takes a value that must be dropped along with it.
_DROPPED_FLAGS_WITH_VALUE = frozenset([
    '-emit-module-path',
    '-emit-objc-header-path',
    '-output-file-map',
    '-file-prefix-map',
])

_NINJA_VAR_RE = re.compile(r'\$\{(\w+)\}|\$(\w+)')
_ENV_ASSIGNMENT_RE = re.compile(r'^\w+=')


class Target:
    """A Swift module built by GN."""

    def __init__(self, label, module_name, sources, ninja_vars, out_dir):
        self.label = label
        self.module_name = module_name
        self.sources = sources
        self.ninja_vars = ninja_vars
        self.out_dir = out_dir

    @property
    def uri(self):
        # `//foo/bar:baz` -> `gn://foo/bar:baz`
        return 'gn:' + self.label

    def __repr__(self):
        return f'<Target {self.label} module={self.module_name}>'


def _unescape_ninja(value):
    return value.replace('$:', ':').replace('$ ', ' ').replace('$$', '$')


def _expand_ninja_vars(template, variables):

    def replace(match):
        name = match.group(1) or match.group(2)
        return variables.get(name, '')

    return _NINJA_VAR_RE.sub(replace, template)


def _parse_rule_command(toolchain_ninja_path, rule_name):
    """Returns the `command` line of `rule <rule_name>` in toolchain.ninja."""
    wanted = f'rule {rule_name}'
    with open(toolchain_ninja_path, encoding='utf8') as stream:
        in_rule = False
        for line in stream:
            line = line.rstrip('\n')
            if not line.startswith((' ', '\t')):
                in_rule = line.strip() == wanted
                continue
            if in_rule:
                stripped = line.strip()
                if stripped.startswith('command = '):
                    return stripped[len('command = '):]
    raise LookupError(f'no `{wanted}` in {toolchain_ninja_path}')


def _parse_target_ninja(path, out_dir):
    """Parses a generated `obj/**/*.ninja` file into a Target, or None.

    Only the variable block and the single `swift` build edge are of interest;
    everything else in the file describes linking.
    """
    variables = {}
    sources = None
    with open(path, encoding='utf8') as stream:
        for line in stream:
            line = line.rstrip('\n')
            if line.startswith('build '):
                # `build <outputs>: swift <inputs> | <implicit> || <order-only>`
                edge = line[len('build '):]
                outputs, _, rest = edge.partition(': ')
                if not rest.startswith('swift '):
                    continue
                del outputs
                inputs = rest[len('swift '):]
                for separator in (' | ', ' || '):
                    inputs = inputs.split(separator)[0]
                sources = [
                    _unescape_ninja(source) for source in inputs.split(' ')
                    if source.endswith('.swift')
                ]
                continue
            name, separator, value = line.partition(' = ')
            if separator and ' ' not in name:
                variables[name] = value

    if not sources or 'module_name' not in variables:
        return None

    label_name = variables.get('label_name')
    target_out_dir = variables.get('target_out_dir', '')
    # `obj/brave/ios/swift` -> `//brave/ios/swift`
    label_dir = '//' + target_out_dir.removeprefix('obj/')
    label = f'{label_dir}:{label_name}'
    return Target(label, variables['module_name'], sources, variables, out_dir)


def find_targets(out_dir):
    """Finds every Swift target in `out_dir` by scanning generated ninja files.

    `gn desc` would be the obvious source of this information but it needs a
    full graph load per invocation, which is far too slow at Chromium scale.
    """
    targets = []
    obj_dir = os.path.join(out_dir, 'obj')
    for root, _, files in os.walk(obj_dir):
        for name in files:
            if not name.endswith('.ninja'):
                continue
            path = os.path.join(root, name)
            # Cheap pre-filter: the swift edge is the only thing being looked
            # for, and it names the sources inline.
            with open(path, 'rb') as stream:
                if b': swift ' not in stream.read():
                    continue
            target = _parse_target_ninja(path, out_dir)
            if target:
                targets.append(target)
    return targets


def _swiftc_py_argv(target):
    """Rebuilds the swiftc.py command line for `target`."""
    toolchain_ninja = os.path.join(target.out_dir, 'toolchain.ninja')
    command = _parse_rule_command(toolchain_ninja, 'swift')
    variables = dict(target.ninja_vars)
    variables['in'] = ' '.join(target.sources)
    expanded = _expand_ninja_vars(command, variables)

    argv = shlex.split(expanded)
    # Strip the leading `FOO=bar ... python3` prefix from the ninja command.
    # The dropped environment assignments (TOOL_VERSION, XCODE_VERSION,
    # DEVELOPER_DIR, ...) only feed swiftc.py's derived-data build signature
    # hash and are never used to assemble the swiftc arguments, so extraction
    # does not need them.
    while argv and _ENV_ASSIGNMENT_RE.match(argv[0]):
        argv.pop(0)
    if argv and os.path.basename(argv[0]).startswith('python'):
        argv.pop(0)
    if argv and argv[0].endswith('swiftc.py'):
        argv.pop(0)
    return argv


# Contents of the fake `swiftc` that swiftc.py is pointed at. It records the
# command line it was invoked with and fails, so swiftc.py stops before its
# post-compile steps (which read files only a real compile produces).
_CAPTURE_SWIFTC = '''\
#!/usr/bin/env python3
# Generated by brave/tools/swift/gn_swift_args.py. Do not edit.
import json, os, sys
with open(os.environ['GN_SWIFT_ARGS_OUTPUT'], 'w', encoding='utf8') as stream:
    json.dump(sys.argv, stream)
sys.exit(1)
'''


def cache_root(out_dir):
    """Returns the directory holding all editor-only build state."""
    return os.path.join(os.path.abspath(out_dir), LSP_CACHE_DIR_NAME)


def _write_capture_toolchain(out_dir):
    """Creates a fake toolchain whose `swiftc` records its arguments.

    swiftc.py builds the compiler path as `<toolchain>/usr/bin/swiftc` from its
    `--swift-toolchain-path` argument, so pointing that at a directory we own is
    enough to intercept the invocation without depending on anything internal to
    swiftc.py.
    """
    toolchain_dir = os.path.join(cache_root(out_dir), 'capture_toolchain')
    swiftc_path = os.path.join(toolchain_dir, 'usr', 'bin', 'swiftc')
    os.makedirs(os.path.dirname(swiftc_path), exist_ok=True)

    existing = None
    if os.path.exists(swiftc_path):
        with open(swiftc_path, encoding='utf8') as stream:
            existing = stream.read()
    if existing != _CAPTURE_SWIFTC:
        with open(swiftc_path, 'w', encoding='utf8') as stream:
            stream.write(_CAPTURE_SWIFTC)
        os.chmod(swiftc_path, 0o700)
    return toolchain_dir


def _swiftc_py_path(source_root):
    """Returns the path to swiftc.py, checking the interception still holds."""
    path = os.path.join(source_root, 'build', 'toolchain', 'apple',
                        'swiftc.py')
    if not os.path.exists(path):
        raise RuntimeError(
            f'{path} does not exist. The Swift toolchain wrapper moved '
            f'upstream; brave/tools/swift/gn_swift_args.py needs updating.')
    with open(path, encoding='utf8') as stream:
        contents = stream.read()
    for expected in ('--swift-toolchain-path', 'usr/bin/swiftc'):
        if expected not in contents:
            raise RuntimeError(
                f'{path} no longer references `{expected}`, so the swiftc '
                f'invocation can no longer be intercepted. '
                f'brave/tools/swift/gn_swift_args.py needs updating.')
    return path


def _extract_swiftc_command(target, source_root):
    """Returns the swiftc command line swiftc.py would run for `target`."""
    out_dir = target.out_dir
    # Keep each module's derived data separate: swiftc.py prunes stale entries
    # from this directory, so sharing it between modules would make them delete
    # each other's module caches.
    derived_data_dir = os.path.join(cache_root(out_dir), 'modules',
                                    target.module_name)
    # swiftc.py writes a handful of bookkeeping files (OutputFileMap.json, the
    # SwiftFileList response file) and creates directories under
    # `--target-out-dir`/`--header-path` before it ever runs the compiler, i.e.
    # before the capture toolchain stops it. Left at their ninja values those
    # would land in the real build directory. Both are documented, required
    # swiftc.py flags and argparse takes the last occurrence, so redirecting
    # them into the cache directory (appended after the ninja arguments) keeps
    # every write swiftc.py makes inside `swift_lsp_cache/`. This is separate
    # from `derived_data_dir`, which swiftc.py prunes.
    scratch_dir = os.path.join(cache_root(out_dir), 'scratch',
                               target.module_name)
    os.makedirs(scratch_dir, exist_ok=True)
    # Deliberately outside `derived_data_dir`: swiftc.py deletes everything in
    # there that it does not recognise.
    output_path = os.path.join(cache_root(out_dir),
                               f'{target.module_name}.swiftc_argv.json')
    os.makedirs(derived_data_dir, exist_ok=True)
    if os.path.exists(output_path):
        os.unlink(output_path)

    command = [
        sys.executable,
        _swiftc_py_path(source_root),
        '--swift-toolchain-path',
        _write_capture_toolchain(out_dir),
        '--derived-data-dir',
        derived_data_dir,
    ] + _swiftc_py_argv(target) + [
        '--target-out-dir',
        os.path.join(scratch_dir, 'target_out'),
        '--header-path',
        os.path.join(scratch_dir, f'{target.module_name}.h'),
    ]

    # Everything in the ninja files is relative to the build directory, and
    # swiftc.py bakes its working directory into `-working-directory`. Running
    # it as a subprocess keeps this out of our own process, whose working
    # directory is global and shared across threads.
    process = subprocess.run(
        command,
        cwd=out_dir,
        env={
            **os.environ, 'GN_SWIFT_ARGS_OUTPUT': output_path
        },
        capture_output=True,
        text=True,
        check=False)

    if not os.path.exists(output_path):
        raise RuntimeError(
            f'could not extract swiftc arguments for {target.label}:\n'
            f'{process.stdout}\n{process.stderr}')

    with open(output_path, encoding='utf8') as stream:
        return json.load(stream)


def _read_swift_file_list(path, out_dir):
    """Expands a `@Module.SwiftFileList` response file into source paths."""
    with open(os.path.join(out_dir, path), encoding='utf8') as stream:
        return [
            os.path.normpath(os.path.join(out_dir, line))
            for line in shlex.split(stream.read())
        ]


def _rewrite_for_indexing(command, out_dir):
    """Turns a build swiftc command line into one usable for indexing."""
    arguments = []
    index = 1  # Skip argv[0], the swiftc path.
    while index < len(command):
        argument = command[index]
        index += 1
        if argument in _DROPPED_FLAGS:
            continue
        if argument in _DROPPED_FLAGS_WITH_VALUE:
            index += 1
            continue
        if argument in _SHARED_CACHE_FLAGS:
            arguments.append(argument)
            arguments.append(
                os.path.join(cache_root(out_dir),
                             _SHARED_CACHE_FLAGS[argument]))
            index += 1  # Discard the per-target path swiftc.py chose.
            continue
        if re.fullmatch(r'-j\d*', argument):
            continue
        if argument == '-num-threads':
            index += 1
            continue
        if argument == '-Xfrontend' and index + 2 < len(
                command) and command[index] == '-const-gather-protocols-file':
            index += 3
            continue
        if argument.startswith('@') and argument.endswith('.SwiftFileList'):
            arguments.extend(_read_swift_file_list(argument[1:], out_dir))
            continue
        arguments.append(argument)
    return arguments


# Cache directories swiftc needs to write into during indexing but does not
# create itself, failing if they are missing. They all resolve inside the
# editor-only cache directory: `-module-cache-path` is redirected to the shared
# module cache by `_rewrite_for_indexing`, while `-index-store-path` and
# `-pch-output-dir` keep the per-target locations swiftc.py chose under
# `--derived-data-dir` (also inside the cache directory).
_CACHE_DIR_FLAGS = frozenset([
    '-index-store-path',
    '-module-cache-path',
    '-pch-output-dir',
])


def ensure_cache_dirs(arguments):
    """Creates the cache directories `arguments` refers to.

    swiftc creates neither its module cache nor its PCH output directory, it
    fails if they are missing. They exist right after extraction, but the
    arguments are cached in memory and outlive them: anything that reclaims disk
    space by deleting the cache directory would otherwise break every open file
    until the editor is restarted.
    """
    for index, argument in enumerate(arguments):
        if argument in _CACHE_DIR_FLAGS and index + 1 < len(arguments):
            os.makedirs(arguments[index + 1], exist_ok=True)


def compiler_arguments(target, source_root):
    """Returns swiftc arguments for indexing every source file in `target`."""
    command = _extract_swiftc_command(target, source_root)
    arguments = _rewrite_for_indexing(command, target.out_dir)
    ensure_cache_dirs(arguments)
    return arguments


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root',
                        default=os.path.abspath(
                            os.path.join(os.path.dirname(__file__), '..', '..',
                                         '..', '..')),
                        help='path to the Chromium src directory')
    parser.add_argument('--out-dir',
                        required=True,
                        help='GN output directory, e.g. out/ios_sim')
    parser.add_argument('--json',
                        action='store_true',
                        help='print the arguments as a JSON array')
    parser.add_argument('file',
                        nargs='?',
                        help='.swift file to print arguments for; if omitted, '
                        'lists all Swift targets')
    args = parser.parse_args(argv)

    source_root = os.path.abspath(args.source_root)
    out_dir = os.path.abspath(os.path.join(source_root, args.out_dir))
    targets = find_targets(out_dir)

    if not args.file:
        for target in sorted(targets, key=lambda target: target.label):
            print(f'{target.label}  module={target.module_name}  '
                  f'{len(target.sources)} source(s)')
        return 0

    wanted = os.path.abspath(args.file)
    for target in targets:
        for source in target.sources:
            if os.path.normpath(os.path.join(out_dir, source)) == wanted:
                arguments = compiler_arguments(target, source_root)
                if args.json:
                    print(json.dumps(arguments, indent=2))
                else:
                    print(f'# target: {target.label}')
                    print(f'# working directory: {out_dir}')
                    print(shlex.join(arguments))
                return 0

    print(f'{wanted} does not belong to any Swift target in {out_dir}',
          file=sys.stderr)
    return 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
