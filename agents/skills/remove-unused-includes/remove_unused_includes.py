#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Removes unused #includes from Brave sources with clang-include-cleaner.

Runs in stages, all state kept in --work-dir so each stage can be resumed:

  analyze  Generates a Brave-only compilation database, runs
           clang-include-cleaner on every eligible .cc file (or .h file, with
           --headers) in parallel and writes plan.json. Removals that may be
           needed by code inactive in this build configuration (e.g.
           `#if BUILDFLAG(IS_WIN)` blocks) are kept.
  fixup    Headers only, before apply: adds the headers about to be removed
           to the Brave files that include an edited header and use them, so
           they no longer rely on the transitive include.
  apply    Deletes the planned includes, backing up every edited file.
  verify   Builds with `pnpm run build --ignore_compile_failure`, puts back
           the includes blamed by compile errors, re-applies, and repeats
           until the build is clean. Edited files whose objects the build did
           not compile are reverted, as they were not verified.
  restore  Reverts every edited file from its backup.
  report   Summarises plan.json.

Usage:
    remove_unused_includes.py analyze [--folder brave/components/foo]
    remove_unused_includes.py apply
    remove_unused_includes.py verify [--build-arg=--gn=foo:true ...]

    remove_unused_includes.py analyze --headers [--folder ...]
    remove_unused_includes.py fixup
    remove_unused_includes.py apply
    remove_unused_includes.py verify [--build-arg=--gn=foo:true ...]
"""

import argparse
import collections
import json
import os
import posixpath
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

# .../src/brave/agents/skills/remove-unused-includes/<this> -> parents[4]
SRC_ROOT = Path(__file__).resolve().parents[4]
EXE = '.exe' if sys.platform == 'win32' else ''
CLEANER = (
    SRC_ROOT
    / 'third_party/llvm-build/Release+Asserts/bin'
    / f'clang-include-cleaner{EXE}'
)

# Path components never analysed: vendored code, and chromium_src overrides,
# which are #included into upstream files rather than compiled on their own.
EXCLUDED_DIRS = {'third_party', 'vendor', 'chromium_src'}

DIRECTIVE = re.compile(r'^\s*#\s*(\w+)')
COND_OPEN = re.compile(r'^\s*#\s*if')
COND_END = re.compile(r'^\s*#\s*endif')
IDENT = re.compile(r'\b[A-Za-z_]\w*\b')
CHANGE = re.compile(r'^- (["<])(.*)[">] @Line:(\d+)$')
ANSI = re.compile(r'\x1b\[[0-9;]*m')
FAILED = re.compile(r'^FAILED: (?:\S+ "\./)?(obj/[^"\s]+\.o)', re.M)
INCLUDE = re.compile(r'^\s*#\s*include\s*"([^"]+)"', re.M)
ERROR_LOCATION = re.compile(r'^\.\./\.\./(\S+?\.h):\d+:\d+: error:', re.M)


class Workspace:
    """Paths and state shared by all stages."""

    def __init__(self, args):
        self.build_dir = SRC_ROOT / 'out' / args.config
        self.work = Path(args.work_dir).resolve()
        self.db_dir = self.work / 'db'
        self.analysis = self.work / 'analysis'
        # Originals, for `restore`.
        self.backup = self.work / 'backup'
        # Contents right before `apply`, which `revert` goes back to.
        self.base = self.work / 'base'
        self.plan_path = self.work / 'plan.json'
        self.deps_path = self.work / 'deps.json'
        self.fixups_path = self.work / 'fixups.json'
        self._db = None
        self._dirs = None

    @property
    def db(self):
        """Maps src-relative source path -> list of compdb entries."""
        if self._db is None:
            self._db = collections.defaultdict(list)
            entries = json.loads(
                (self.db_dir / 'compile_commands.json').read_text()
            )
            for e in entries:
                path = os.path.normpath(os.path.join(e['directory'], e['file']))
                self._db[rel(path)].append(e)
        return self._db

    @property
    def sources_by_dir(self):
        """Maps directory -> sorted .cc files in it with a compdb entry."""
        if self._dirs is None:
            self._dirs = collections.defaultdict(list)
            for path in sorted(self.db):
                if path.endswith('.cc'):
                    self._dirs[posixpath.dirname(path)].append(path)
        return self._dirs

    def load_plan(self):
        return json.loads(self.plan_path.read_text())

    def save_plan(self, plan):
        self.plan_path.write_text(json.dumps(plan, indent=1, sort_keys=True))

    def backup_of(self, rel_path):
        return self.backup / rel_path

    def base_of(self, rel_path):
        return self.base / rel_path


def rel(path):
    return Path(os.path.relpath(path, SRC_ROOT)).as_posix()


def save_copy(path, dest):
    dest.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(SRC_ROOT / path, dest)


def files_under(root):
    if not root.exists():
        return set()
    return {
        f.relative_to(root).as_posix() for f in root.rglob('*') if f.is_file()
    }


# ---------------------------------------------------------------------------
# analyze
# ---------------------------------------------------------------------------


def generate_compdb(ws):
    ws.db_dir.mkdir(parents=True, exist_ok=True)
    full = ws.work / 'compile_commands.full.json'
    print(f'Generating compilation database for {rel(ws.build_dir)}...')
    subprocess.run(
        [
            sys.executable,
            'tools/clang/scripts/generate_compdb.py',
            '-p',
            str(ws.build_dir),
            '-o',
            str(full),
        ],
        cwd=SRC_ROOT,
        check=True,
    )
    # clang-include-cleaner parses the whole database on every run, so keep
    # only Brave's entries to make each invocation fast.
    entries = [
        e
        for e in json.loads(full.read_text())
        if rel(os.path.join(e['directory'], e['file'])).startswith('brave/')
    ]
    (ws.db_dir / 'compile_commands.json').write_text(json.dumps(entries))
    full.unlink()


def excluded(path):
    return path.startswith('out/') or bool(EXCLUDED_DIRS & set(path.split('/')))


def changed_files(base):
    """src-relative files changed since the merge-base with |base|."""
    brave = SRC_ROOT / 'brave'
    merge_base = subprocess.run(
        ['git', 'merge-base', 'HEAD', base],
        cwd=brave,
        capture_output=True,
        text=True,
        check=True,
    ).stdout.strip()
    names = subprocess.run(
        ['git', 'diff', '--name-only', '--diff-filter=AMR', merge_base],
        cwd=brave,
        capture_output=True,
        text=True,
        check=True,
    ).stdout.split()
    return {f'brave/{n}' for n in names}


def eligible_files(ws, folders, headers, only=None):
    prefixes = [rel(Path(f).resolve()).rstrip('/') + '/' for f in folders]
    if headers:
        candidates = []
        for folder in folders:
            for root, dirs, names in os.walk(folder):
                dirs[:] = [d for d in dirs if d not in EXCLUDED_DIRS]
                candidates += [
                    rel(os.path.join(root, n))
                    for n in names
                    if n.endswith('.h')
                ]
        candidates = [c for c in sorted(set(candidates)) if donor(ws, c)]
    else:
        candidates = [p for p in sorted(ws.db) if p.endswith('.cc')]
    return [
        p
        for p in candidates
        if not excluded(p)
        and any(p.startswith(x) for x in prefixes)
        and (only is None or p in only)
    ]


def donor(ws, header):
    """The .cc whose compile flags stand in for |header|'s."""
    stem = header[: -len('.h')] + '.cc'
    if stem in ws.db:
        return stem
    directory = posixpath.dirname(header)
    while directory:
        if sources := ws.sources_by_dir.get(directory):
            return sources[0]
        # Headers inside a Rust crate belong to the crate, not to Brave.
        if (SRC_ROOT / directory / 'Cargo.toml').exists():
            return None
        directory = posixpath.dirname(directory)
    return None


def analysis_file(ws, path):
    return ws.analysis / (path.replace('/', '_') + '.txt')


def run_cleaner(ws, path):
    """Runs clang-include-cleaner once per file, caching its output."""
    out = analysis_file(ws, path)
    if out.exists():
        return
    res = subprocess.run(
        [
            str(CLEANER),
            '-p',
            str(ws.db_dir),
            '--print=changes',
            '--disable-insert',
            path,
        ],
        cwd=SRC_ROOT,
        capture_output=True,
        text=True,
        check=False,
    )
    if res.returncode != 0:
        print(f'  [skip] {path}: {res.stderr.strip().splitlines()[-1:]}')
        out.write_text('')
        return
    out.write_text(res.stdout)


def compile_args(entry):
    """Compile command for |entry| without its output/dependency flags."""
    if 'arguments' in entry:
        args = list(entry['arguments'])
    else:
        args = shlex.split(entry['command'], posix=sys.platform != 'win32')
    res = []
    skip = False
    for a in args:
        if skip:
            skip = False
        elif a in ('-MF', '-o'):
            skip = True
        elif a not in ('-MMD', '-MD', '-c'):
            res.append(a)
    return res


def compile_args_for(ws, path):
    """Compile command for |path|, borrowed from a donor for headers."""
    if path in ws.db:
        return compile_args(ws.db[path][0])
    source = donor(ws, path)
    if not source:
        return None
    source_arg = Path(os.path.relpath(SRC_ROOT / source, ws.build_dir))
    args = [
        a
        for a in compile_args(ws.db[source][0])
        if a != source_arg.as_posix() and not a.startswith('-fmodule-name=')
    ]
    header_arg = Path(os.path.relpath(SRC_ROOT / path, ws.build_dir))
    return args + ['-x', 'c++-header', header_arg.as_posix()]


def include_dirs(ws, args):
    dirs = []
    for i, a in enumerate(args):
        if a in ('-I', '-isystem', '-iquote') and i + 1 < len(args):
            dirs.append(args[i + 1])
        elif a.startswith(('-I', '-iquote')) and a not in ('-I', '-iquote'):
            dirs.append(a[2:] if a.startswith('-I') else a[7:])
        elif a.startswith('-isystem') and a != '-isystem':
            dirs.append(a[8:])
    return [os.path.normpath(ws.build_dir / d) for d in dirs]


def active_lines(ws, path, args):
    """Line numbers of |path| that survive preprocessing, or None."""
    res = subprocess.run(
        args + ['-E', '-o', '-'],
        cwd=ws.build_dir,
        capture_output=True,
        text=True,
        check=False,
    )
    if res.returncode != 0:
        return None
    target = os.path.normpath(SRC_ROOT / path)
    marker = re.compile(r'^# (\d+) "(.*)"')
    active = set()
    in_target = False
    line = 0
    for text in res.stdout.splitlines():
        m = marker.match(text)
        if m:
            line = int(m.group(1))
            in_target = os.path.normpath(ws.build_dir / m.group(2)) == target
            continue
        if in_target and text.strip():
            active.add(line)
        line += 1
    return active


def inactive_identifiers(lines, active):
    """Identifiers in conditional branches that produced no output."""
    n = len(lines)
    starts = []
    i = 0
    while i < n:
        if DIRECTIVE.match(lines[i]):
            starts.append(i)
            while lines[i].rstrip().endswith('\\') and i + 1 < n:
                i += 1
        i += 1
    segments = []
    stack = []
    for i in starts:
        d = DIRECTIVE.match(lines[i]).group(1)
        if d in ('if', 'ifdef', 'ifndef'):
            stack.append(i)
        elif d in ('elif', 'elifdef', 'elifndef', 'else') and stack:
            segments.append((stack[-1], i))
            stack[-1] = i
        elif d == 'endif' and stack:
            segments.append((stack.pop(), i))
    inactive = set()
    for start, end in segments:
        body = range(start + 1, end)
        if not any(k + 1 in active for k in body):
            inactive.update(body)
    text = '\n'.join(lines[k] for k in sorted(inactive))
    return set(IDENT.findall(re.sub(r'//.*', '', text)))


_header_cache = {}


def header_identifiers(path):
    """Approximate set of names a header declares, or None if unreadable."""
    if path in _header_cache:
        return _header_cache[path]
    try:
        t = Path(path).read_text(encoding='utf-8', errors='replace')
    except OSError:
        _header_cache[path] = None
        return None
    t = re.sub(r'//.*', '', t)
    ids = set()
    ids |= set(
        re.findall(
            r'\b(?:class|struct|union|enum(?:\s+class)?)\s+'
            r'(?:[A-Z_]+\([^)]*\)\s+)?(\w+)',
            t,
        )
    )
    ids |= set(re.findall(r'#\s*define\s+(\w+)', t))
    ids |= set(re.findall(r'BUILDFLAG_INTERNAL_(\w+)\(', t))
    ids |= set(re.findall(r'\busing\s+(\w+)\s*=', t))
    ids |= set(re.findall(r'\btypedef\b[^;]*?\b(\w+)\s*;', t))
    ids |= set(re.findall(r'\b(k[A-Z]\w*)\b', t))
    ids |= set(re.findall(r'\b([A-Z][A-Za-z0-9]*[a-z]\w*)\s*\(', t))
    ids |= set(re.findall(r'\b(IDS?_[A-Z0-9_]+|IDR_[A-Z0-9_]+)\b', t))
    ids -= {'class', 'struct', 'enum', 'final', 'override'}
    ids = {i for i in ids if not i.endswith('_H_')}
    _header_cache[path] = ids
    return ids


def resolve_header(include, angled, path, dirs):
    candidates = [] if angled else [str((SRC_ROOT / path).parent)]
    for d in candidates + dirs:
        p = os.path.join(d, include)
        if os.path.isfile(p):
            return p
    return None


def plan_file(ws, path):
    """Splits the cleaner's suggestions for |path| into remove and keep."""
    changes = []
    for text in analysis_file(ws, path).read_text().splitlines():
        m = CHANGE.match(text)
        if m:
            changes.append(
                {
                    'inc': m.group(2),
                    'angled': m.group(1) == '<',
                    'line': int(m.group(3)),
                }
            )
    if not changes:
        return None
    lines = (SRC_ROOT / path).read_text(encoding='utf-8').split('\n')
    args = compile_args_for(ws, path)
    if args is None:
        return None
    dirs = include_dirs(ws, args)
    inactive = set()
    if any(COND_OPEN.match(t) for t in lines):
        active = active_lines(ws, path, args)
        if active is None:
            return None
        inactive = inactive_identifiers(lines, active)
    remove, keep = [], []
    for c in changes:
        if c['inc'] not in lines[c['line'] - 1]:
            keep.append({**c, 'why': 'line mismatch'})
            continue
        if inactive:
            hp = resolve_header(c['inc'], c['angled'], path, dirs)
            ids = header_identifiers(hp) if hp else None
            if ids is None:
                keep.append({**c, 'why': 'unresolved header'})
                continue
            if hit := sorted(ids & inactive):
                keep.append(
                    {**c, 'why': 'inactive code: ' + ', '.join(hit[:5])}
                )
                continue
        remove.append(c)
    return {
        'kind': 'header' if path.endswith('.h') else 'source',
        'remove': remove,
        'keep': keep,
    }


def cmd_analyze(ws, args):
    if not CLEANER.exists():
        sys.exit(f'clang-include-cleaner not found at {CLEANER}')
    if (
        args.regenerate_compdb
        or not (ws.db_dir / 'compile_commands.json').exists()
    ):
        generate_compdb(ws)
    ws.analysis.mkdir(parents=True, exist_ok=True)
    only = changed_files(args.base) if args.branch else None
    files = eligible_files(ws, args.folder, args.headers, only)
    print(f'Analysing {len(files)} files with {args.jobs} jobs...')
    with ThreadPoolExecutor(args.jobs) as ex:
        for i, _ in enumerate(ex.map(lambda f: run_cleaner(ws, f), files)):
            if (i + 1) % 100 == 0:
                print(f'  {i + 1}/{len(files)}')
    plan = {}
    with ThreadPoolExecutor(args.jobs) as ex:
        for path, entry in zip(
            files, ex.map(lambda f: plan_file(ws, f), files)
        ):
            if entry:
                plan[path] = entry
    ws.save_plan(plan)
    cmd_report(ws)


# ---------------------------------------------------------------------------
# fixup
# ---------------------------------------------------------------------------


def load_header_deps(ws, headers):
    """Maps each of |headers| to the objects whose recorded deps list it."""
    wanted = {
        Path(os.path.relpath(SRC_ROOT / h, ws.build_dir)).as_posix(): h
        for h in headers
    }
    if (ws.build_dir / '.siso_deps').exists():
        cmd = ['siso', 'query', 'deps']
    else:
        cmd = ['ninja', '-t', 'deps']
    deps = collections.defaultdict(set)
    obj = None
    with subprocess.Popen(
        cmd, cwd=ws.build_dir, stdout=subprocess.PIPE, text=True
    ) as proc:
        for line in proc.stdout:
            if not line.startswith(' '):
                obj = line.split(':', 1)[0]
            elif header := wanted.get(line.strip()):
                deps[header].add(obj)
    return {h: sorted(objs) for h, objs in deps.items()}


def only_headers_regex(removals):
    return ','.join(
        '(^|/)' + re.escape(inc) + '$'
        for inc in sorted({r['inc'] for r in removals})
    )


def insert_includes(ws, path, removals=None):
    """Lets the cleaner add the headers |path| uses but does not include.

    Only the |removals| headers are considered, or every header if None.
    """
    src = SRC_ROOT / path
    before = src.read_bytes()
    cmd = [str(CLEANER), '-p', str(ws.db_dir), '--edit', '--disable-remove']
    if removals is not None:
        cmd.append(f'--only-headers={only_headers_regex(removals)}')
    res = subprocess.run(
        cmd + [path], cwd=SRC_ROOT, capture_output=True, check=False
    )
    after = src.read_bytes()
    if res.returncode != 0 or after == before:
        src.write_bytes(before)
        return False
    if not ws.backup_of(path).exists():
        ws.backup_of(path).parent.mkdir(parents=True, exist_ok=True)
        ws.backup_of(path).write_bytes(before)
    return True


def cmd_fixup(ws, args):
    plan = ws.load_plan()
    headers = {
        p: e['remove']
        for p, e in plan.items()
        if e.get('kind') == 'header' and e['remove']
    }
    if applied := [h for h in headers if ws.base_of(h).exists()]:
        sys.exit(f'Run fixup before apply; already applied: {applied[:3]}')
    print(f'Reading recorded deps for {len(headers)} headers...')
    deps = load_header_deps(ws, headers)
    ws.deps_path.write_text(json.dumps(deps, indent=1, sort_keys=True))

    # Includers: compiled Brave sources from the deps log, plus Brave headers
    # that include an edited header directly.
    obj_to_src = {e['output']: p for p, es in ws.db.items() for e in es}
    needs = collections.defaultdict(list)
    for header, objs in deps.items():
        for obj in objs:
            if (src := obj_to_src.get(obj)) and src != header:
                needs[src] += headers[header]
    for root, dirs, names in os.walk(SRC_ROOT / 'brave'):
        dirs[:] = [d for d in dirs if d not in EXCLUDED_DIRS]
        for name in names:
            if not name.endswith('.h'):
                continue
            path = rel(os.path.join(root, name))
            text = Path(root, name).read_text(
                encoding='utf-8', errors='replace'
            )
            for inc in INCLUDE.findall(text):
                if inc in headers and inc != path:
                    needs[path] += headers[inc]
    # Edited headers include everything they use themselves: clang modules
    # require it, and it covers headers relying on another edited header.
    print(f'Adding direct includes to {len(headers)} edited headers...')
    with ThreadPoolExecutor(args.jobs) as ex:
        changed = sum(ex.map(lambda h: insert_includes(ws, h), headers))
    print(f'Added includes to {changed} edited headers.')
    includers = {
        p: r for p, r in needs.items() if not excluded(p) and p not in headers
    }
    print(f'Adding direct includes to {len(includers)} includers...')
    with ThreadPoolExecutor(args.jobs) as ex:
        edited = [
            p
            for p, changed in zip(
                includers,
                ex.map(
                    lambda p: insert_includes(ws, p, includers[p]), includers
                ),
            )
            if changed
        ]
    ws.fixups_path.write_text(json.dumps(sorted(edited), indent=1))
    print(f'Added includes to {len(edited)} includers.')


# ---------------------------------------------------------------------------
# apply / restore
# ---------------------------------------------------------------------------


def tidy(lines):
    """Drops emptied #if blocks and collapses leftover blank lines."""
    changed = True
    while changed:
        changed = False
        for i, text in enumerate(lines):
            if not COND_OPEN.match(text):
                continue
            j = i + 1
            while j < len(lines) and not lines[j].strip():
                j += 1
            if j < len(lines) and COND_END.match(lines[j]):
                del lines[i : j + 1]
                changed = True
                break
    out = []
    for text in lines:
        blank = not text.strip()
        if blank and out and (not out[-1].strip() or COND_OPEN.match(out[-1])):
            continue
        if (
            COND_END.match(text)
            and len(out) > 1
            and not out[-1].strip()
            and out[-2].lstrip().startswith('#include')
        ):
            out.pop()
        out.append(text)
    return out


def include_line(lines, removal):
    """Index of |removal|'s #include in |lines|, or None."""
    inc = re.escape(removal['inc'])
    spelled = f'<{inc}>' if removal.get('angled') else f'"{inc}"'
    directive = re.compile(r'^\s*#\s*include\s*' + spelled)
    return next((i for i, t in enumerate(lines) if directive.match(t)), None)


def remove_includes(lines, drop):
    new = [t for i, t in enumerate(lines) if i not in drop]
    # Only tidy the include block around the removals.
    first = max(0, min(drop) - 1)
    end = 1 + max(
        (i for i, t in enumerate(new) if t.lstrip().startswith('#include')),
        default=first,
    )
    while end < len(new) and (COND_END.match(new[end]) or not new[end].strip()):
        end += 1
    return new[:first] + tidy(new[first:end]) + new[end:]


def cmd_apply(ws):
    plan = ws.load_plan()
    edited = skipped = 0
    for path, entry in sorted(plan.items()):
        if not entry['remove'] or ws.base_of(path).exists():
            continue
        src = SRC_ROOT / path
        lines = src.read_text(encoding='utf-8').split('\n')
        drop = {include_line(lines, r) for r in entry['remove']}
        if None in drop:
            print(f'  [skip] {path} changed since analysis')
            skipped += 1
            continue
        save_copy(path, ws.base_of(path))
        if not ws.backup_of(path).exists():
            save_copy(path, ws.backup_of(path))
        src.write_text(
            '\n'.join(remove_includes(lines, drop)), encoding='utf-8'
        )
        edited += 1
    print(f'Edited {edited} files, skipped {skipped}.')


def revert(ws, plan, path, removals, why):
    """Undoes `apply` on |path| and moves |removals| from remove to keep."""
    if not ws.base_of(path).exists():
        return
    entry = plan[path]
    entry['keep'] += [{**r, 'why': why} for r in removals]
    entry['remove'] = [r for r in entry['remove'] if r not in removals]
    shutil.copy2(ws.base_of(path), SRC_ROOT / path)
    ws.base_of(path).unlink()


def cmd_restore(ws):
    originals = files_under(ws.backup)
    for path in originals:
        shutil.copy2(ws.backup_of(path), SRC_ROOT / path)
        ws.backup_of(path).unlink()
    for path in files_under(ws.base):
        ws.base_of(path).unlink()
    print(f'Restored {len(originals)} files.')


# ---------------------------------------------------------------------------
# verify
# ---------------------------------------------------------------------------


def run_build(ws, args, round_number):
    pnpm = shutil.which('pnpm') or 'pnpm'
    cmd = [
        pnpm,
        'run',
        'build',
        args.config,
        '--ignore_compile_failure',
        f'--target={args.target}',
    ] + args.build_arg
    log = ws.work / f'build{round_number}.log'
    print(f'Round {round_number}: {shlex.join(cmd[1:])} > {log}')
    with log.open('w') as f:
        rc = subprocess.run(
            cmd,
            cwd=SRC_ROOT / 'brave',
            stdout=f,
            stderr=subprocess.STDOUT,
            check=False,
        ).returncode
    return rc, ANSI.sub('', log.read_text(errors='replace'))


def error_identifiers(chunk):
    ids = set()
    for text in chunk.splitlines():
        if 'error:' in text:
            for quoted in re.findall(r"'([^']*)'", text):
                ids |= set(IDENT.findall(quoted))
    return ids


def matching_removals(ws, path, ids, removals):
    """Removals whose header declares one of |ids|."""
    dirs = include_dirs(ws, compile_args_for(ws, path) or [])
    matched = []
    for r in removals:
        hp = resolve_header(r['inc'], r.get('angled', False), path, dirs)
        hid = header_identifiers(hp) if hp else None
        if hid and hid & ids:
            matched.append(r)
    return matched


def blame(ws, plan, edited, obj, src, chunk, obj_headers):
    """Maps edited file -> removals blamed for the failure in |chunk|."""
    ids = error_identifiers(chunk)
    if src in plan and src in edited and plan[src].get('kind') != 'header':
        removals = plan[src]['remove']
        return {src: matching_removals(ws, src, ids, removals) or removals}
    # A failure in a file we did not remove from is caused by an edited
    # header it includes, or by an edited header named in the errors.
    headers = set(obj_headers.get(obj, ()))
    headers |= set(ERROR_LOCATION.findall(chunk))
    headers = {
        h
        for h in headers
        if h in plan and h in edited and plan[h].get('kind') == 'header'
    }
    blamed = {
        h: m
        for h in headers
        if (m := matching_removals(ws, h, ids, plan[h]['remove']))
    }
    return blamed or {h: plan[h]['remove'] for h in headers}


def cmd_verify(ws, args):
    obj_to_src = {e['output']: path for path, es in ws.db.items() for e in es}
    deps = json.loads(ws.deps_path.read_text()) if ws.deps_path.exists() else {}
    obj_headers = collections.defaultdict(list)
    for header, objs in deps.items():
        for obj in objs:
            obj_headers[obj].append(header)
    for round_number in range(1, args.max_rounds + 1):
        rc, log = run_build(ws, args, round_number)
        if rc == 0:
            break
        plan = ws.load_plan()
        edited = files_under(ws.base)
        blamed = collections.defaultdict(list)
        unexplained = []
        for chunk in re.split(r'(?m)(?=^FAILED: )', log):
            if not (m := FAILED.match(chunk)):
                continue
            obj = m.group(1)
            src = obj_to_src.get(obj)
            found = blame(ws, plan, edited, obj, src, chunk, obj_headers)
            if not found:
                unexplained.append(obj)
            for path, removals in found.items():
                blamed[path] += [r for r in removals if r not in blamed[path]]
        if not blamed:
            sys.exit(
                f'Build failed outside the edited files ({unexplained[:3]}); '
                'fix it and rerun verify. See '
                f'{ws.work}/build{round_number}.log'
            )
        for path, removals in blamed.items():
            revert(ws, plan, path, removals, 'build failure')
        ws.save_plan(plan)
        print(f'  {len(blamed)} edited files blamed; re-applying the rest.')
        cmd_apply(ws)
    else:
        sys.exit(f'Build still failing after {args.max_rounds} rounds.')

    # Anything the build did not compile after the edit is unverified. Added
    # includes cannot break the build, so fixups are not checked.
    plan = ws.load_plan()
    unverified = []
    for path in sorted(files_under(ws.base)):
        src_mtime = (SRC_ROOT / path).stat().st_mtime
        if plan[path].get('kind') == 'header':
            objs = deps.get(path, [])
        else:
            objs = [e['output'] for e in ws.db[path]]
        if not objs or any(
            not (ws.build_dir / o).exists()
            or (ws.build_dir / o).stat().st_mtime < src_mtime
            for o in objs
        ):
            unverified.append(path)
    for path in unverified:
        print(f'  [revert] {path} not compiled by {args.target}')
        revert(ws, plan, path, plan[path]['remove'], 'not compiled')
    ws.save_plan(plan)
    cmd_report(ws)


# ---------------------------------------------------------------------------
# report
# ---------------------------------------------------------------------------


def cmd_report(ws):
    plan = ws.load_plan()
    applied = files_under(ws.base)
    changed = files_under(ws.backup)
    removals = sum(len(e['remove']) for e in plan.values())
    reasons = collections.Counter(
        k['why'].split(':')[0] for e in plan.values() for k in e['keep']
    )
    fixups = (
        json.loads(ws.fixups_path.read_text())
        if ws.fixups_path.exists()
        else []
    )
    print(
        f'{len(plan)} files with suggestions, {removals} planned removals, '
        f'{len(applied)} files edited, '
        f'{len(changed & set(fixups))} includers with added includes.'
    )
    for why, n in reasons.most_common():
        print(f'  kept {n}: {why}')


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument(
        'stage',
        choices=['analyze', 'fixup', 'apply', 'verify', 'restore', 'report'],
    )
    parser.add_argument(
        '--branch',
        action='store_true',
        help='Only analyse files changed since the merge-base with --base, '
        'including uncommitted changes.',
    )
    parser.add_argument(
        '--base',
        default='origin/master',
        help='Base for --branch (default: origin/master).',
    )
    parser.add_argument(
        '--headers',
        action='store_true',
        help='Analyse .h files instead of .cc files.',
    )
    parser.add_argument(
        '--config', default='Component', help='Build config under out/.'
    )
    parser.add_argument(
        '--work-dir',
        default=os.path.join(
            tempfile.gettempdir(), 'brave_remove_unused_includes'
        ),
        help='Where the database, plan, logs and backups are kept.',
    )
    parser.add_argument(
        '--folder',
        action='append',
        help='Folder to analyse, repeatable (default: the whole brave tree).',
    )
    parser.add_argument(
        '--jobs', type=int, default=max(1, min(56, (os.cpu_count() or 2) // 2))
    )
    parser.add_argument('--regenerate-compdb', action='store_true')
    parser.add_argument('--target', default='brave:all')
    parser.add_argument(
        '--build-arg',
        action='append',
        default=[],
        help='Extra `pnpm run build` argument, repeatable; pass the same '
        '--gn flags the build directory was generated with.',
    )
    parser.add_argument('--max-rounds', type=int, default=10)
    args = parser.parse_args()
    args.folder = args.folder or [str(SRC_ROOT / 'brave')]

    ws = Workspace(args)
    ws.work.mkdir(parents=True, exist_ok=True)
    {
        'analyze': lambda: cmd_analyze(ws, args),
        'fixup': lambda: cmd_fixup(ws, args),
        'apply': lambda: cmd_apply(ws),
        'verify': lambda: cmd_verify(ws, args),
        'restore': lambda: cmd_restore(ws),
        'report': lambda: cmd_report(ws),
    }[args.stage]()


if __name__ == '__main__':
    main()
