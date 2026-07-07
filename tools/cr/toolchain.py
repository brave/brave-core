# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Hermetic toolchains handled during a Chromium upgrade.
"""

from __future__ import annotations

import ast
from dataclasses import dataclass
import os
import re
import textwrap

import requests

from ci import JenkinsCi
from exceptions import BadOutcomeException, InvalidInputException
from git_status import GitStatus
import repository
from terminal import terminal
from toolchains import build_rust_toolchain, build_xcode_toolchain
from versioning import Version

# ---------------------------------------------------------------------------
# This is a simple schema with all the data necessary to manage the hermetic
# toolchains we may have to update during a Chromium upgrade.
#
#   source  -- the Chromium files this toolchain is pinned by, plus the
#              constants read from them: a change to any of them flags a new
#              toolchain is required, and their current values identify the
#              commit that set them (the culprit).
#   ci      -- how to trigger the job(s): the job URL(s) plus either a
#              `build_param` (the tag is sent in that build parameter) or a
#              `properties` tuple (the field names the job reads from a JSON
#              PROPERTIES payload).
#   publish -- (optional) `{revision}`-templated URL of the published artifact,
#              used to tell whether CI already built this toolchain.
#   repin   -- (optional) the in-tree pin the `repin` command rewrites and any
#              upstream file it mirrors. Absent when there is no automated
#              repin.
#   advice  -- guidance shown in the pre-run advisory.
#
# `ToolchainSpec` is a thin typed view over one entry.
# ---------------------------------------------------------------------------

TOOLCHAINS = {
    'windows': {
        'label': 'Windows SDK',
        'source': {
            'files': ('build/vs_toolchain.py', ),
            'keys': ('SDK_VERSION', 'TOOLCHAIN_HASH'),
        },
        'ci': {
            'build_param': 'CHROMIUM_TAG',
            'jobs': ('https://ci.brave.com/view/toolchains/job/'
                     'windows-hermetic-toolchain-build/', ),
        },
        'advice': ('Contact DevOps regarding the new WinSDK for the hermetic '
                   'toolchain. Update `env.GYP_MSVS_HASH_*` in '
                   'build/commands/lib/config.js with correct hashes.'),
    },
    'xcode': {
        'label': 'macOS SDK',
        'source': {
            'files': ('build/config/mac/mac_sdk.gni', ),
            'keys': ('mac_sdk_official_version',
                     'mac_sdk_official_build_version'),
        },
        'ci': {
            'build_param': 'CHROMIUM_TAG',
            'jobs': ('https://ci.brave.com/view/toolchains/job/'
                     'xcode-hermetic-toolchain-build/', ),
        },
        'repin': {
            'script': 'build/mac/download_hermetic_xcode.py',
            'upstream_min_os_file': 'build/mac_toolchain.py',
        },
        'advice': (
            'Contact DevOps to ask for an updated macOS toolchain node to '
            'be used to generate a new toolchain, then generate the new '
            'toolchain in https://ci.brave.com/view/toolchains/. Once the '
            'new toolchain is published, call '
            '`brockit.py update-xcode-toolchain --to=<chromium-ref>` to '
            'repin it.'),
    },
    'rust': {
        'label': 'Rust toolchain',
        'source': {
            'files': ('tools/rust/update_rust.py',
                      'tools/clang/scripts/update.py'),
            'keys': ('RUST_REVISION', 'RUST_SUB_REVISION', 'CLANG_REVISION'),
        },
        'ci': {
            'jobs': (
                'https://ci.brave.com/view/toolchains/job/'
                'brave-browser-rust-toolchain-aux-build-linux-x64/',
                'https://ci.brave.com/view/toolchains/job/'
                'brave-browser-rust-toolchain-aux-build-macos-arm64/',
                'https://ci.brave.com/view/toolchains/job/'
                'brave-browser-rust-toolchain-aux-build-macos-x64/',
                'https://ci.brave.com/view/toolchains/job/'
                'brave-browser-rust-toolchain-aux-build-windows-x64/',
            ),
            # The Rust jobs take no build parameter; they read a JSON
            # PROPERTIES payload with these fields instead. `chromium_ref` is
            # filled from the triggered version; the rest are supplied by the
            # caller of `trigger`.
            'properties': ('brave_subrevision', 'chromium_ref'),
        },
        'publish': {
            # `-1` is the first brave sub-revision: if any toolchain exists for
            # a revision, this archive does. `{revision}` is the Rust/Clang
            # triple `is_published` fills in.
            'url': ('https://brave-build-deps-public.s3.brave.com/'
                    'rust-toolchain-aux/'
                    'linux-x64-rust-toolchain-{revision}-1.tar.xz'),
        },
        'repin': {
            'installer': 'tools/cr/install_extra_deps.py',
        },
        'advice': ('Run the jobs in https://ci.brave.com/view/toolchains/ to '
                   'generate a new Rust toolchain (or use '
                   '`brockit.py gen-rust-toolchain`).'),
    },
}


@dataclass(frozen=True)
class ToolchainSpec:
    """A typed view over one entry of the `TOOLCHAINS` literal.

    `TOOLCHAINS` (above) is the single source of truth for every toolchain
    particular; this dataclass is just syntactic sugar that flattens one nested
    entry into typed, autocomplete-friendly attributes. Construct it with
    `ToolchainSpec.from_entry`.
    """

    # Machine key (e.g. 'rust') and human label (e.g. 'Rust toolchain').
    key: str
    label: str

    # Chromium files diffed/pickaxed for this toolchain, and the constants read
    # from them: a change to any flags that a new toolchain is required, and
    # their current values identify the commit that set them (the culprit).
    files: tuple[str, ...]
    keys: tuple[str, ...]

    # The CI job URLs to trigger.
    job_urls: tuple[str, ...]

    # Guidance shown in the pre-run advisory when a new toolchain is needed.
    advice: str

    # How the job receives the Chromium tag: exactly one of these is set.
    #   build_param -- the tag is sent in this Jenkins build parameter.
    #   properties  -- the job takes no build parameter and instead reads a JSON
    #                  `PROPERTIES` payload; this is the tuple of field names it
    #                  expects (e.g. `('brave_subrevision', 'chromium_ref')`).
    build_param: str | None = None
    properties: tuple[str, ...] | None = None

    # Availability probe: a `{revision}`-templated URL for the published
    # artifact, letting `is_published` suppress the advisory once CI uploads it.
    # None when no probe exists (the advisory then always fires).
    published_url: str | None = None

    # Repin particulars (absent for toolchains with no automated repin):
    #   installer            -- Brave file whose pin the Rust `repin` rewrites.
    #   script               -- Brave file whose pin the Xcode `repin` rewrites.
    #   upstream_min_os_file -- Chromium file the Xcode `repin` mirrors the
    #                           `MAC_MINIMUM_OS_VERSION` block from.
    installer: str | None = None
    script: str | None = None
    upstream_min_os_file: str | None = None

    @classmethod
    def from_entry(cls, key: str, entry: dict) -> ToolchainSpec:
        """Flattens one `TOOLCHAINS` entry into a `ToolchainSpec`."""
        source = entry['source']
        job = entry['ci']
        publish = entry.get('publish') or {}
        repin = entry.get('repin') or {}
        return cls(key=key,
                   label=entry['label'],
                   files=source['files'],
                   keys=source['keys'],
                   job_urls=job['jobs'],
                   advice=entry['advice'],
                   build_param=job.get('build_param'),
                   properties=job.get('properties'),
                   published_url=publish.get('url'),
                   installer=repin.get('installer'),
                   script=repin.get('script'),
                   upstream_min_os_file=repin.get('upstream_min_os_file'))


def get_assigned_value(contents: str,
                       lookup: str,
                       added: bool = False,
                       removed: bool = False) -> str | None:
    """Uses a basic regex to extract the value being assigned to a key.

    This helper extracts the value of a key from file contents that is being
    diffed or read in general.

    Args:
        contents:
            The contents of the file to look for the key.
        lookup:
            The key to look for in the contents.
        added:
            If set to True, looks for the key starting with +.
        removed:
            If set to True, looks for the key starting with -.
    Returns:
        The value assigned to the key, or None if not found.
    """
    for line in contents.splitlines():
        # Discard any comment that there may be in the line.
        line = line.split('#', 1)[0].strip()

        if lookup not in line:
            continue
        if added and not line.startswith('+'):
            continue
        if removed and not line.startswith('-'):
            continue

        # Remove the sign at the beginning of the line, if that's the case we
        # are looking for.
        if added or removed:
            line = line[1:]

        if '=' not in line:
            continue

        [key, value] = line.split('=', 1)
        if key.strip().lstrip() == lookup:
            return value.strip().lstrip().replace('"', '').replace("'", "")
    return None


class Toolchain:
    """Base class for a hermetic toolchain, driven by its `ToolchainSpec`."""

    def __init__(self, spec: ToolchainSpec) -> None:
        self.spec = spec

    # -- detection ----------------------------------------------------------

    def _diff(self, working: Version | str, target: Version | str) -> str:
        """Diffs this toolchain's files across the `working..target` range."""
        return repository.chromium.run_git('diff', str(working), str(target),
                                           '--', *self.spec.files)

    def was_updated(self, working: Version | str,
                    target: Version | str) -> bool:
        """Whether any detection constant changed across the range.
        """
        diff = self._diff(working, target)
        return any(
            get_assigned_value(diff, key, added=True) is not None
            or get_assigned_value(diff, key, removed=True) is not None
            for key in self.spec.keys)

    def is_published(self, target: Version | str) -> bool:
        """Whether a built toolchain is already available for `target`.

        This function can be overridden for custom checks.
        """
        del target
        return False

    def _revision(self, ref: Version | str) -> str:
        """A human-readable identifier of the toolchain pinned at `ref`.

        Joins the detection constants' values, e.g. the Rust triple
        `RUST_REVISION-RUST_SUB_REVISION-CLANG_REVISION` or the Windows
        `SDK_VERSION-TOOLCHAIN_HASH`.
        """
        text = repository.chromium.read_file(*self.spec.files, commit=str(ref))
        return '-'.join(
            get_assigned_value(text, key) or '?' for key in self.spec.keys)

    # -- culprit attribution (shared by all toolchain types) ----------------

    @staticmethod
    def _raw_assignment(text: str, key: str) -> str | None:
        """Returns the verbatim right-hand side of `key = <value>` in `text`.

        Unlike `get_assigned_value`, quotes are preserved and leading
        indentation is tolerated (e.g. `mac_sdk.gni` nests its assignments),
        so the value can be pickaxed against the file exactly as written.
        """
        match = re.search(rf'(?m)^\s*{re.escape(key)} = (.+)$', text)
        if not match:
            return None
        return match.group(1).split('#', 1)[0].strip()

    def _culprit_regex(self, ref: Version | str) -> str | None:
        """Builds the `git log -G` alternation identifying the culprit commit.

        Reads the culprit constants' verbatim values at `ref` and joins them as
        `key = <escaped value>` alternatives. Returns None when none of the
        constants can be read.
        """
        text = repository.chromium.read_file(*self.spec.files, commit=str(ref))
        parts = []
        for key in self.spec.keys:
            value = self._raw_assignment(text, key)
            if value is not None:
                parts.append(f'{key} = {re.escape(value)}')
        return '|'.join(parts) if parts else None

    def _pickaxe(self,
                 ref: Version | str,
                 since: Version | str | None = None) -> tuple[str, str]:
        """Returns the `(hash, subject)` of the culprit commit, best-effort.

        Pickaxes the culprit files for the commit that set their current values
        at `ref`, optionally restricted to the `since..ref` range. Returns
        empty strings when nothing matches.
        """
        regex = self._culprit_regex(ref)
        if not regex:
            return '', ''
        range_arg = f'{since}..{ref}' if since is not None else str(ref)
        out = repository.chromium.run_git('log', '--extended-regexp', '-G',
                                          regex, '--pretty=%H%x00%s', '-1',
                                          range_arg, '--', *self.spec.files)
        commit_hash, _, subject = out.partition('\x00')
        return commit_hash, subject

    def find_culprit(self,
                     ref: Version | str,
                     culprit: str | None = None) -> str:
        """The single culprit-resolution method used by every toolchain.

        Returns `culprit` verbatim when provided; otherwise pickaxes the
        culprit files for the Chromium commit that set their current values at
        `ref`.
        """
        if culprit:
            return culprit
        commit_hash, _ = self._pickaxe(ref)
        if not commit_hash:
            raise InvalidInputException(
                f'Could not find the Chromium commit pinning the '
                f'{self.spec.label} toolchain at {ref}. Pass '
                '[bold cyan]--culprit[/] to point at it explicitly.')
        return commit_hash

    # -- advisory (consumed by brockit's pre-run checks) --------------------

    def check(self, working: Version | str,
              target: Version | str) -> ToolchainAdvisory | None:
        """Returns an advisory when a new toolchain is needed, else None.

        A new toolchain is needed when a detection constant changed across the
        range and no built artifact is already published for `target`.
        """
        if not self.was_updated(working, target):
            return None
        if self.is_published(target):
            return None

        commit_hash, subject = self._pickaxe(target, since=working)
        return ToolchainAdvisory(
            description=(f'{self.spec.label} has been updated. '
                         f'{self._revision(working)} ➜ '
                         f'{self._revision(target)}'),
            advice=self.spec.advice,
            commit_hash=commit_hash,
            commit_message=subject)

    # -- CI launch (shared by all toolchain types) --------------------------

    def trigger(self,
                version: Version | str,
                *,
                watch: bool = False,
                **properties) -> bool:
        """Triggers this toolchain's CI job(s) for `version`.

        Toolchains with a `build_param` carry the tag there and take no
        `properties`. Toolchains that declare `spec.properties` instead receive
        a JSON `PROPERTIES` payload: `chromium_ref` (when declared) defaults to
        the triggered version, and every other declared field must be supplied
        here as a keyword argument.

        Returns whether the pipelines finished successfully (see
        `ci.JenkinsCi.trigger`): always True when not watching.
        """
        # Toolchains with a build parameter carry the tag there; the ones that
        # take a PROPERTIES payload (see `spec.properties`) send no build param.
        params = {
            self.spec.build_param: str(version)
        } if self.spec.build_param else {}
        return JenkinsCi.from_config().trigger(
            self.spec.job_urls,
            params=params,
            properties=self._properties_payload(version, properties),
            watch=watch,
            title=f'{self.spec.label} · Chromium {version}')

    def _properties_payload(self, version: Version | str,
                            provided: dict) -> dict | None:
        """Builds the `PROPERTIES` payload, checking every field is provided.

        `chromium_ref` (when the toolchain declares it) defaults to the
        triggered version; all other declared fields must come from `provided`.
        Raises if a toolchain that takes no properties is given any, or if the
        supplied fields don't match exactly what the toolchain declares.
        """
        if self.spec.properties is None:
            if provided:
                raise InvalidInputException(
                    f'The {self.spec.label} toolchain takes no properties.')
            return None

        payload = dict(provided)
        if 'chromium_ref' in self.spec.properties:
            payload.setdefault('chromium_ref', str(version))
        if set(payload) != set(self.spec.properties):
            raise InvalidInputException(
                f'The {self.spec.label} toolchain requires the properties '
                f'{sorted(self.spec.properties)}; got {sorted(payload)}.')
        return payload

    # -- recovery (auto-resolve a needed toolchain during a lift) -----------

    def recover(self, target: Version, culprit: str) -> bool:
        """Best-effort auto-resolution of a needed toolchain during a lift.

        The idea is to close the loop without the user: kick off the CI job(s),
        wait for them, and repin the freshly published result. Returns True when
        fully recovered (nothing left for the user), so the caller can drop the
        advisory.

        The base class cannot recover automatically and returns False.
        """
        del target, culprit
        return False

    # -- repin (in-tree pin + commit), overridden where applicable ----------

    def repin(self, version: Version, culprit: str | None = None) -> None:
        """Repins the in-tree pin to the published toolchain and commits it."""
        del version, culprit
        raise InvalidInputException(
            f'The {self.spec.label} toolchain has no automated repin.')

    @staticmethod
    def _require_no_staged_files() -> None:
        """Guards repin against clobbering an in-progress staged change."""
        status = GitStatus()
        if status.has_staged_files():
            raise InvalidInputException(
                'Staged files detected. Please commit or unstage changes '
                'before generating a toolchain update:\n%s' %
                '\n'.join(status.get_all_staged_entries()))


@dataclass(frozen=True)
class ToolchainAdvisory:
    """A pre-run advisory that a new toolchain is required."""

    # One-line summary of what changed (e.g. "Rust toolchain has been updated.
    # <old> ➜ <new>").
    description: str

    # Guidance on how to resolve it by hand (the toolchain's `spec.advice`).
    advice: str

    # The Chromium commit that introduced the change, and its subject line --
    # rendered as the attributed culprit in the advisory.
    commit_hash: str
    commit_message: str


class RustToolchain(Toolchain):
    """The Rust/WASM toolchain, pinned in `install_extra_deps.py`."""

    # A recovery is the first build for a revision, so it triggers (and probes,
    # via the `publish` URL) brave sub-revision 1.
    _FIRST_BRAVE_SUBREVISION = 1

    def __init__(self) -> None:
        super().__init__(ToolchainSpec.from_entry('rust', TOOLCHAINS['rust']))

    def is_published(self, target: Version | str) -> bool:
        """Whether the published Rust archive already exists for `target`.

        Suppresses the advisory once CI has built and uploaded the toolchain.
        """
        url = self.spec.published_url.format(revision=self._revision(target))
        try:
            response = requests.head(url, allow_redirects=True, timeout=5)
            return response.status_code == 200
        except requests.RequestException:
            # Assume the toolchain is not available if the request fails.
            return False

    def recover(self, target: Version, culprit: str) -> bool:
        """Trigger the Rust jobs, watch them, and repin on success.

        Any failure returns False so the caller keeps the advisory for the user
        to resolve.
        """
        try:
            if not self.trigger(
                    target,
                    watch=True,
                    brave_subrevision=self._FIRST_BRAVE_SUBREVISION):
                return False
            self.repin(target, culprit)
        except (InvalidInputException, BadOutcomeException):
            return False
        return True

    @staticmethod
    def _load_extra_deps(source: str) -> tuple[ast.Assign, dict]:
        """Return the `EXTRA_DEPS = {...}` assignment node and its dict value.

        The value is read with `ast.literal_eval`, so the installer is parsed
        as data rather than imported (importing it pulls in gclient).
        """
        for node in ast.parse(source).body:
            if (isinstance(node, ast.Assign) and len(node.targets) == 1
                    and isinstance(node.targets[0], ast.Name)
                    and node.targets[0].id == 'EXTRA_DEPS'):
                return node, ast.literal_eval(node.value)
        raise InvalidInputException(
            'No EXTRA_DEPS assignment found in the Rust installer.')

    @staticmethod
    def _upstream_stem(text: str) -> str:
        """Build the upstream toolchain package stem from the revision scripts.

        Matches `package_rust.RUST_TOOLCHAIN_PACKAGE_NAME` minus the `.tar.xz`
        suffix --
        `rust-toolchain-<RUST_REVISION>-<RUST_SUB_REVISION>-<CLANG_REVISION>` --
        built from the scripts' source so brockit needn't import `package_rust`
        (which pulls in gclient). Value patterns mirror
        `tools/clang/scripts/upload_revision.py`: quoted strings, bare-integer
        sub-revision.
        """

        def read(key: str, value: str) -> str:
            match = re.search(rf'{key} = {value}', text)
            if not match:
                raise InvalidInputException(
                    f'{key} not found in the Rust revision scripts.')
            return match.group(1)

        quoted, integer = r"'([0-9a-z-]+)'", r'([0-9]+)'
        return ('rust-toolchain-'
                f'{read("RUST_REVISION", quoted)}-'
                f'{read("RUST_SUB_REVISION", integer)}-'
                f'{read("CLANG_REVISION", quoted)}')

    @staticmethod
    def _commit_title(new_entry: dict) -> str:
        """Build a commit subject naming the exact toolchain build being pinned.

        We build the title to show the new toolchain's relevant details, being
        the rust version, and the clang revision it was built with. The
        trailing `sub` is the upstream rust sub revision (RUST_SUB_REVISION),
        not Brave's respin counter. The result is something like this:

          Rust/WASM toolchain (4c4205163abc-5, llvmorg-23-init-10931-g20b6ec77, sub 5)

        The values in the title are extracted from the tarball's name.
        """
        object_name = next(iter(
            new_entry.values()))['objects'][0]['object_name']
        name = object_name.removesuffix('.tar.xz')
        match = re.search(
            r'rust-toolchain-(?P<rust>[0-9a-f]+)-(?P<sub>\d+)-'
            r'(?P<clang>llvmorg-.+)-(?P<brave>\d+)$', name)
        if not match:
            return f'Rust/WASM toolchain {name}'
        return (f'Rust/WASM toolchain ({match["rust"][:12]}-{match["sub"]}, '
                f'{match["clang"]}, sub {match["sub"]})')

    def repin(self, version: Version, culprit: str | None = None) -> None:
        """Repins the Rust/WASM `EXTRA_DEPS` entry and commits it."""
        self._require_no_staged_files()

        ref = str(version)
        revision_text = repository.chromium.read_file(*self.spec.files,
                                                      commit=ref)
        upstream_stem = self._upstream_stem(revision_text)

        try:
            new_entry = build_rust_toolchain.rust_toolchain_extra_dep(
                upstream_stem)
        except RuntimeError as e:
            raise BadOutcomeException(str(e)) from e

        installer = self.spec.installer
        path = repository.brave.root / installer
        source = path.read_bytes().decode('utf-8')
        node, extra_deps = self._load_extra_deps(source)

        # Replace the rust-toolchain entry (in place, preserving key order),
        # then re-render the whole EXTRA_DEPS assignment.
        extra_deps.update(new_entry)
        rendered = f'EXTRA_DEPS = {_render_py_literal(extra_deps)}\n'
        lines = source.splitlines(keepends=True)
        new_source = (''.join(lines[:node.lineno - 1]) + rendered +
                      ''.join(lines[node.end_lineno:]))

        if new_source == source:
            terminal.log_task(
                f'{installer} is already up to date; nothing to commit.')
            return

        path.write_text(new_source, encoding='utf-8', newline='')

        commit_hash = self.find_culprit(ref, culprit)
        repository.brave.run_git('add', installer)
        repository.brave.git_commit(self._commit_title(new_entry),
                                    env={
                                        **os.environ,
                                        'tags': 'toolchain',
                                        'culprit': commit_hash,
                                    })


class XcodeToolchain(Toolchain):
    """The hermetic Xcode toolchain, pinned in `download_hermetic_xcode.py`."""

    # Matches the provenance comment above the pinned constants in the hermetic
    # Xcode script so it can be regenerated from the freshly resolved toolchain.
    # Spans the opening line and any contiguous comment lines that follow,
    # stopping at the first non-comment line (the constants).
    _PROVENANCE_COMMENT_RE = re.compile(
        r'^# This contains binaries from Xcode\b.*\n(?:#.*\n)*', re.MULTILINE)

    # Matches the `MAC_MINIMUM_OS_VERSION` block, with its leading comment lines
    # plus the assignment, in both Chromium's `build/mac_toolchain.py` and the
    # hermetic downloader. The Xcode `repin` lifts this block from upstream and
    # drops it into the downloader verbatim. Each block is preceded by a blank
    # line, so the contiguous comment run never reaches further up.
    _MIN_OS_VERSION_BLOCK_RE = re.compile(
        r'(?:^#.*\n)*^MAC_MINIMUM_OS_VERSION = \[[^\]]*\]\n', re.MULTILINE)

    def __init__(self) -> None:
        super().__init__(ToolchainSpec.from_entry('xcode',
                                                  TOOLCHAINS['xcode']))

    @staticmethod
    def _provenance_comment(sdk_info: build_xcode_toolchain.MacSdkInfo,
                            index: dict) -> str:
        """Render the wrapped provenance comment for the resolved toolchain.

        Records, the Xcode version/build and Metal build the index reports
        alongside the macOS SDK version/build, so the in-tree downloader keeps
        documenting exactly what its archive contains now that it no longer
        carries an `XCODE_VERSION` constant.
        """
        metal_build = index.get('metal_build') or 'unknown'
        sentence = (
            f"This contains binaries from Xcode {index['xcode_version']} "
            f"({index['xcode_build']}) along with the macOS "
            f"{sdk_info.sdk_version} SDK ({sdk_info.product_build_version}) "
            f"and the Metal toolchain ({metal_build}).")
        return textwrap.fill(
            sentence, width=79, initial_indent='# ',
            subsequent_indent='# ') + '\n'

    def _rewrite_hermetic_xcode_script(
            self, sdk_info: build_xcode_toolchain.MacSdkInfo, index: dict,
            mac_toolchain_py: str) -> bool:
        """Pins the archive hash and SDK constants, refreshes the provenance
        comment, and mirrors Chromium's `MAC_MINIMUM_OS_VERSION` block.

        `mac_toolchain_py` is the upstream `build/mac_toolchain.py` source the
        minimum-OS block is lifted from.

        Returns True if the on-disk file actually changed, False when it was
        already pinned to these exact values (so the caller can skip
        committing).
        """
        script_path = repository.brave.root / self.spec.script
        original = script_path.read_bytes().decode('utf-8')
        sdk_version = sdk_info.sdk_version
        build_version = sdk_info.product_build_version

        content = re.sub(r"MAC_BINARIES_HASH = '[^']*'",
                         f"MAC_BINARIES_HASH = '{index['sha256sum']}'",
                         original,
                         count=1)
        content = re.sub(r"MAC_SDK_OFFICIAL_VERSION = '[^']*'",
                         f"MAC_SDK_OFFICIAL_VERSION = '{sdk_version}'",
                         content,
                         count=1)
        content = re.sub(r"MAC_SDK_OFFICIAL_BUILD_VERSION = '[^']*'",
                         f"MAC_SDK_OFFICIAL_BUILD_VERSION = '{build_version}'",
                         content,
                         count=1)
        content = self._PROVENANCE_COMMENT_RE.sub(
            lambda _: self._provenance_comment(sdk_info, index),
            content,
            count=1)

        # Lift the minimum-OS gate (and its comment) verbatim from upstream.
        upstream_min_os = self._MIN_OS_VERSION_BLOCK_RE.search(
            mac_toolchain_py)
        if upstream_min_os is None:
            raise InvalidInputException(
                'Could not find the MAC_MINIMUM_OS_VERSION block in '
                f'{self.spec.upstream_min_os_file}.')
        content = self._MIN_OS_VERSION_BLOCK_RE.sub(
            lambda _: upstream_min_os.group(0), content, count=1)

        if content == original:
            return False

        script_path.write_text(content, encoding='utf-8', newline='')
        return True

    def repin(self, version: Version, culprit: str | None = None) -> None:
        """Repins `download_hermetic_xcode.py` and commits it."""
        self._require_no_staged_files()

        ref = str(version)
        mac_sdk_gni = repository.chromium.read_file(self.spec.files[0],
                                                    commit=ref)
        mac_toolchain_py = repository.chromium.read_file(
            self.spec.upstream_min_os_file, commit=ref)
        try:
            sdk_info = build_xcode_toolchain.MacSdkInfo.from_gni(mac_sdk_gni)
        except RuntimeError as e:
            raise BadOutcomeException(str(e)) from e

        try:
            index = build_xcode_toolchain.fetch_published_index(sdk_info)
        except RuntimeError as e:
            raise BadOutcomeException(str(e)) from e

        if not self._rewrite_hermetic_xcode_script(sdk_info, index,
                                                   mac_toolchain_py):
            raise InvalidInputException(
                f'{self.spec.script} is already pinned to these values; '
                'nothing to commit.')

        commit_hash = self.find_culprit(ref, culprit)
        title = (f'Switch to Xcode {index["xcode_version"]} '
                 f'{index["xcode_build"]}')
        repository.brave.run_git('add', self.spec.script)
        repository.brave.git_commit(title,
                                    env={
                                        **os.environ,
                                        'tags': 'toolchain',
                                        'culprit': commit_hash,
                                    })


class WindowsToolchain(Toolchain):
    """The hermetic Windows SDK / VS toolchain.

    Detection and CI triggering are fully data-driven via the base class; the
    in-tree pin lives in a Brave build script that has no automated repin, so
    `repin` falls back to the base class's "not automated" error.
    """

    def __init__(self) -> None:
        super().__init__(
            ToolchainSpec.from_entry('windows', TOOLCHAINS['windows']))


def _render_py_literal(value: object, indent: int = 0) -> str:
    """Render a str/bool/dict/list literal as multi-line Python source.

    Matches the layout `install_extra_deps.py` already uses -- 4-space
    indent steps, single-quoted strings, trailing commas -- so re-rendering
    `EXTRA_DEPS` round-trips through yapf unchanged.
    """
    pad = ' ' * indent
    child = ' ' * (indent + 4)
    if isinstance(value, dict):
        if not value:
            return '{}'
        items = ''.join(f'{child}{_render_py_literal(key)}: '
                        f'{_render_py_literal(val, indent + 4)},\n'
                        for key, val in value.items())
        return f'{{\n{items}{pad}}}'
    if isinstance(value, list):
        if not value:
            return '[]'
        items = ''.join(f'{child}{_render_py_literal(item, indent + 4)},\n'
                        for item in value)
        return f'[\n{items}{pad}]'
    if isinstance(value, bool):
        return 'True' if value else 'False'
    if isinstance(value, str):
        # Single-quoted to match the file. Values here never contain a single
        # quote or backslash; fall back to repr only defensively.
        if "'" in value or '\\' in value:
            return repr(value)
        return f"'{value}'"
    return repr(value)
