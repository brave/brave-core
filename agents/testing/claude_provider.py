#!/usr/bin/env python3

# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""A promptfoo `python:` provider that drives Claude Code for skill evals.

This is the Brave/Claude-Code analog of chromium's `gemini_provider.py`. It
mirrors that contract — reads `config.skills`, `config.changes`,
`config.timeoutSeconds` — but the agent under test is the Claude Code CLI, since
our skills use Claude Code-only frontmatter and are not runnable under
gemini-cli.

promptfoo calls `call_api(prompt, options, context)` and expects a dict with an
`output` (str) or an `error` (str), plus optional `metrics`.

Per-test provider `config` keys:
    timeoutSeconds:  int   run timeout (default 1200)
    skills:          [str] skills that must be discoverable (linked by
                           agents/skills/setup.py); verified before the run
    allowedTools:    str   value for `claude --allowedTools` (default below)
    changes:         [ {apply: patch} | {stage: path} ]   git fixups (optional)
    fake_gh:         {...} canned-data object for fake_gh.py (see that file).
                           When present, a stub `gh` is put first on PATH so the
                           run never touches real GitHub.

Isolation notes:
  * The run executes with cwd = brave-core root so the skill's repo-root
    detection and docs/best-practices lookups resolve correctly.
  * TMPDIR is pointed at a fresh per-run dir; skills that mkdtemp their work
    dir land inside it, so we can harvest their `*_results.json` afterwards and
    write a merged, deterministic artifact at agents/testing/.last_run/ that
    asserts can read without knowing the random temp path.

Open items (need a live, authenticated Claude Code run to finalize — see the
spec's open questions): the exact headless auth Claude Code needs in CI, and
confirming the harvested results path against a real review-prs run.
"""

import glob
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

_TESTING_DIR = Path(__file__).resolve().parent
_BRAVE_SRC = _TESTING_DIR.parents[1]           # .../src/brave
_SKILLS_SRC = _BRAVE_SRC / 'agents' / 'skills'
_LAST_RUN_DIR = _TESTING_DIR / '.last_run'
_SETUP_PY = _SKILLS_SRC / 'setup.py'

DEFAULT_TIMEOUT_SECONDS = 1200
DEFAULT_ALLOWED_TOOLS = 'Bash,Read,Glob,Grep,Write,Edit,Task'
# A Claude Code --allowedTools value: comma-separated tool names, some with a
# parenthesised scope like `Bash(gh pr diff:*)`. Validated before use so no
# unexpected data reaches the subprocess argv.
_ALLOWED_TOOLS_RE = re.compile(r'^[A-Za-z0-9_,:()*.\- /]*$')
# A resolvable program name or path — no shell metacharacters/whitespace.
_BIN_RE = re.compile(r'^[A-Za-z0-9_./\-]+$')


def _resolve_claude_bin():
    """Resolve and validate the Claude Code binary.

    $CLAUDE_BIN is the only value fed to subprocess that originates from the
    environment, so validate it here (reject metacharacters, require it to
    resolve to a real executable) instead of trusting it. Returns an absolute
    path; raises ValueError on anything suspicious or unresolvable.
    """
    raw = os.environ.get('CLAUDE_BIN', 'claude')
    if not _BIN_RE.match(raw):
        raise ValueError(f'invalid CLAUDE_BIN (unexpected characters): {raw!r}')
    resolved = shutil.which(raw) or (raw if os.path.isfile(raw)
                                     and os.access(raw, os.X_OK) else None)
    if not resolved:
        raise ValueError(f'CLAUDE_BIN not found or not executable: {raw!r}')
    return os.path.abspath(resolved)


def _ensure_skills_linked(skills):
    """Make sure requested skills are discoverable in .claude/skills/."""
    if _SETUP_PY.exists():
        subprocess.run([sys.executable, str(_SETUP_PY), 'link', '-q'],
                       cwd=str(_BRAVE_SRC), check=False)
    missing = [s for s in (skills or [])
               if not (_SKILLS_SRC / s).is_dir()]
    if missing:
        raise FileNotFoundError(
            f'Requested skill(s) not found under {_SKILLS_SRC}: {missing}')


def _apply_changes(changes, cwd):
    for change in changes or []:
        if len(change) != 1:
            raise ValueError('Invalid change: must have exactly one key.')
        if 'apply' in change:
            subprocess.check_call(['git', 'apply', change['apply']], cwd=cwd)
        elif 'stage' in change:
            subprocess.check_call(['git', 'add', change['stage']], cwd=cwd)
        else:
            raise ValueError('change key must be "apply" or "stage".')


def _resolve_diff_file(path):
    """Resolve a fixture diff path to absolute, robust to invocation cwd.

    Tries: the path as given / relative to cwd, then relative to each eval
    directory. This lets both `run_evals.py` (cwd = config dir) and a direct
    `npx promptfoo eval -c <file>` from src/brave find the fixture.
    """
    if os.path.isabs(path) and os.path.exists(path):
        return path
    candidates = [path]
    for d in (_BRAVE_SRC / 'agents' / 'prompts' / 'eval').glob('**/'):
        candidates.append(str(d / path))
    for c in candidates:
        if os.path.exists(c):
            return os.path.abspath(c)
    return os.path.abspath(path)  # let fake_gh report the miss


def _write_fake_gh(fake_gh_cfg, run_dir):
    """Create a bin dir with a `gh` shim; return (bin_dir, mutations_file)."""
    bin_dir = run_dir / 'bin'
    bin_dir.mkdir(parents=True, exist_ok=True)
    # Resolve every PR's diff_file to an absolute path so the stub finds it
    # regardless of the cwd it runs under.
    fake_gh_cfg = json.loads(json.dumps(fake_gh_cfg))  # deep copy
    for pr in (fake_gh_cfg.get('prs') or {}).values():
        if pr.get('diff_file'):
            pr['diff_file'] = _resolve_diff_file(pr['diff_file'])
    data_file = run_dir / 'fake_gh_data.json'
    mut_file = run_dir / 'gh_mutations.jsonl'
    with open(data_file, 'w', encoding='utf-8') as f:
        json.dump(fake_gh_cfg, f)
    fake_gh_py = _TESTING_DIR / 'fake_gh.py'
    shim = bin_dir / 'gh'
    shim.write_text(
        '#!/usr/bin/env bash\n'
        f'export FAKE_GH_DATA={shlex.quote(str(data_file))}\n'
        f'export FAKE_GH_MUTATIONS={shlex.quote(str(mut_file))}\n'
        f'exec {shlex.quote(sys.executable)} '
        f'{shlex.quote(str(fake_gh_py))} "$@"\n',
        encoding='utf-8')
    shim.chmod(0o755)
    return bin_dir, mut_file


def _harvest_results(run_dir):
    """Merge every subagent *_results.json under run_dir into .last_run/."""
    _LAST_RUN_DIR.mkdir(parents=True, exist_ok=True)
    result_files = sorted(glob.glob(str(run_dir / '**' / 'pr_*' /
                                        '*_results.json'), recursive=True))
    merged = {'result_files': [], 'violations': []}
    for rf in result_files:
        try:
            with open(rf, encoding='utf-8') as f:
                data = json.load(f)
        except (OSError, json.JSONDecodeError):
            continue
        merged['result_files'].append(rf)
        # review-prs result files hold a list of violations (schema varies);
        # keep the raw payload so string asserts (CS-003, symbol name) work.
        merged['violations'].append(data)
    out = _LAST_RUN_DIR / 'results.json'
    with open(out, 'w', encoding='utf-8') as f:
        json.dump(merged, f, indent=2)
    return out, result_files


def call_api(prompt, options, context):
    config = (options or {}).get('config', {})
    try:
        timeout = int(config.get('timeoutSeconds', DEFAULT_TIMEOUT_SECONDS))
    except (TypeError, ValueError):
        return {'error': f'bad timeoutSeconds: {config.get("timeoutSeconds")}'}

    allowed_tools = config.get('allowedTools', DEFAULT_ALLOWED_TOOLS)
    if not _ALLOWED_TOOLS_RE.match(allowed_tools):
        return {'error': f'invalid allowedTools value: {allowed_tools!r}'}

    try:
        claude_bin = _resolve_claude_bin()
    except ValueError as e:
        return {'error': str(e)}

    try:
        _ensure_skills_linked(config.get('skills'))
    except FileNotFoundError as e:
        return {'error': str(e)}

    run_dir = Path(tempfile.mkdtemp(prefix='brave-skill-eval-'))
    env = os.environ.copy()
    env['TMPDIR'] = str(run_dir)
    # Strip the parent Claude Code session markers so the nested headless run
    # starts clean (also correct when this provider itself runs under CI/an
    # outer agent). Harmless when unset.
    for k in ('CLAUDECODE', 'CLAUDE_CODE_CHILD_SESSION', 'CLAUDE_CODE_SESSION_ID',
              'CLAUDE_CODE_ENTRYPOINT', 'CLAUDE_CODE_SSE_PORT'):
        env.pop(k, None)

    mut_file = None
    if config.get('fake_gh'):
        bin_dir, mut_file = _write_fake_gh(config['fake_gh'], run_dir)
        env['PATH'] = f'{bin_dir}{os.pathsep}{env.get("PATH", "")}'
        # review-prs' prepare-review.py exits unless an org-members file exists.
        # In offline evals there's no org roster to consult (single-PR mode
        # skips org filtering anyway), so point it at an empty temp file.
        org_members_file = run_dir / 'org-members.txt'
        org_members_file.write_text('', encoding='utf-8')
        env['BRAVE_ORG_MEMBERS_PATH'] = str(org_members_file)

    try:
        _apply_changes(config.get('changes'), cwd=str(_BRAVE_SRC))
    except (subprocess.CalledProcessError, ValueError) as e:
        return {'error': f'failed to apply changes: {e}'}

    # shell=False list form: argv elements are passed to execve directly, never
    # to a shell, so `prompt`/`allowed_tools` cannot inject a command. The only
    # environment-derived element, claude_bin, is validated by
    # _resolve_claude_bin() above; allowed_tools is regex-checked. Inputs come
    # from checked-in eval config, not remote/untrusted data.
    cmd = [claude_bin, '-p', prompt, '--allowedTools', allowed_tools]
    metrics = {'user_prompt': prompt, 'run_dir': str(run_dir),
               'command': ' '.join(shlex.quote(c) for c in cmd)}
    start = time.time()
    try:
        # nosemgrep: python.lang.security.audit.dangerous-subprocess-use-tainted-env-args.dangerous-subprocess-use-tainted-env-args
        proc = subprocess.run(cmd, cwd=str(_BRAVE_SRC), env=env, text=True,
                              capture_output=True, timeout=timeout)
    except subprocess.TimeoutExpired:
        return {'error': f'Claude Code timed out after {timeout}s',
                'metrics': metrics}
    except FileNotFoundError:
        return {'error': f"Claude Code binary '{claude_bin}' not found.",
                'metrics': metrics}
    metrics['duration'] = time.time() - start

    output = (proc.stdout or '') + (proc.stderr or '')
    collected, result_files = _harvest_results(run_dir)
    metrics['collected_results'] = str(collected)
    metrics['result_files'] = result_files
    if mut_file and os.path.exists(mut_file):
        with open(mut_file, encoding='utf-8') as f:
            metrics['gh_mutations'] = [json.loads(x) for x in f if x.strip()]

    if proc.returncode != 0:
        return {'error': f'Claude Code exited {proc.returncode}.\n{output}',
                'metrics': metrics}
    return {'output': output.strip(), 'metrics': metrics}


# Allow a quick smoke test: `python3 claude_provider.py "<prompt>"`.
if __name__ == '__main__':
    p = sys.argv[1] if len(sys.argv) > 1 else 'Say hello.'
    print(json.dumps(call_api(p, {'config': {}}, {}), indent=2, default=str))
