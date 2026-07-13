# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""All logic w.r.t. the recipe engine's support for protobufs.

Recipes declare typed properties in a sibling `.proto` file and import the
generated message from the `PB` namespace, e.g.

    # recipes/toolchains/rust/package_rust.proto  ->  package
    # recipes.brave.toolchains.rust.package_rust
    from PB.recipes.brave.toolchains.rust.package_rust import InputProperties

`ensure_compiled` gathers every `.proto`, and, when their collective hash
changes, runs `protoc` to (re)build the `PB` package, rewriting the generated
imports to be rooted at `PB`. `append_to_syspath` then makes `PB` importable.
"""

from __future__ import annotations

import ast
import contextlib
from dataclasses import dataclass, field
import errno
import hashlib
import inspect
import mmap
import os
import posixpath
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Callable

import google.protobuf  # pinned in .vpython3
import google.protobuf.message
from google.protobuf import descriptor_pb2

# Root of the recipes tree (this file's directory).
RECIPES_ROOT = Path(__file__).resolve().parent

PROTOC_VERSION = google.protobuf.__version__.encode('utf-8')

# Namespace name this single repo's protos live under, e.g. a recipe proto at
# `recipes/foo/bar.proto` compiles to package `recipes.<REPO_NAME>.foo.bar`.
REPO_NAME = 'brave'


class BadProtoDefinitions(Exception):
    """Raised when the repo has conflicting .proto files."""


if sys.platform.startswith('win'):
    _BAT = '.bat'

    def _to_posix(path: str) -> str:
        return path.replace('\\', '/')
else:
    _BAT = ''

    def _to_posix(path: str) -> str:
        return path


def _file_checksum(path: str | Path) -> str:
    path = Path(path)
    csum = hashlib.sha1()
    csum.update(f'blob {path.stat().st_size}'.encode())
    csum.update(b'\0')
    # mmap hands the whole file to the hash as one buffer; it also raises on an
    # empty file, which we never expect to checksum (a proto has content).
    with path.open('rb') as ins, \
            mmap.mmap(ins.fileno(), 0, access=mmap.ACCESS_READ) as data:
        csum.update(data)
    return csum.hexdigest()


def _write_text(path: Path, text: str) -> None:
    """Write *text* as UTF-8 with no newline translation.

    Follows the file-writing convention in tools/cr/python_dos_and_donts.md so
    generated files stay byte-identical across platforms. `Path.write_text`'s
    `newline` argument is Python 3.10+, which the presubmit's older pylint
    doesn't model, so its false positive is silenced here in one place.
    """
    # pylint: disable=unexpected-keyword-arg
    path.write_text(text, encoding='utf-8', newline='')


@dataclass(frozen=True, order=True)
class _ProtoInfo:
    """Holds information about a proto file found in the repo."""
    # Native-slash-delimited path to the source file.
    src_abspath: str = field(compare=False)

    # The fwd-slash-delimited path relative to `RECIPES_ROOT` of the proto file.
    relpath: str

    # The fwd-slash-delimited path relative to the output PB directory of where
    # this file should go when we compile protos.
    dest_relpath: str

    # The git blob hash of this file.
    blobhash: str = field(compare=False)

    @classmethod
    def create(cls, scan_relpath: str, dest_namespace: str,
               relpath: str) -> _ProtoInfo:
        """Creates a _ProtoInfo.

        This converts `relpath` into a global relpath for the output PB folder,
        according to the `dest_namespace`.

        Args:
          * scan_relpath - The fwd-slash-delimited base path we were scanning
            (relative to `RECIPES_ROOT`), e.g. 'recipes/'.
          * dest_namespace - The fwd-slash-delimited path prefix for the
            destination proto, e.g. 'recipes/brave/'.
          * relpath - The fwd-slash-delimited relative path from `RECIPES_ROOT`
            to where we found the proto, e.g. 'recipes/subdir/something.proto'.
        """
        assert '\\' not in scan_relpath, (
            'scan_relpath must be fwd-slash-delimited: %r' % scan_relpath)
        assert '\\' not in dest_namespace, (
            'dest_namespace must be fwd-slash-delimited: %r' % dest_namespace)
        assert '\\' not in relpath, (
            'relpath must be fwd-slash-delimited: %r' % relpath)

        subpath = relpath[len(scan_relpath):]
        dest_relpath = dest_namespace + subpath
        src_abspath = os.path.normpath(os.path.join(RECIPES_ROOT, relpath))
        blobhash = _file_checksum(src_abspath)

        return cls(src_abspath, relpath, dest_relpath, blobhash)


def _gather_proto_info() -> list[_ProtoInfo]:
    """Gathers all protos from the repo, as a sorted list of _ProtoInfo."""
    # Tuples of:
    #   * fwd-slash path relative to RECIPES_ROOT of where to look for protos.
    #   * fwd-slash namespace prefix of where these protos should go in the
    #     global PB namespace.
    scan_path = [
        ('recipes/', 'recipes/%s/' % REPO_NAME),
        ('recipe_modules/', 'recipe_modules/%s/' % REPO_NAME),
    ]

    ret = []
    for scan_relpath, dest_namespace in scan_path:
        for base, dirs, fnames in os.walk(
                os.path.join(RECIPES_ROOT, scan_relpath)):
            base = str(base)

            # Skip all '.expected' directories.
            dirs[:] = [
                dname for dname in dirs if not dname.endswith('.expected')
            ]

            # fwd-slash relative-to-RECIPES_ROOT version of `base`.
            relbase = _to_posix(os.path.relpath(base, RECIPES_ROOT))

            for fname in fnames:
                fname = str(fname)
                relpath = posixpath.join(relbase, fname)
                if os.path.splitext(relpath)[1] == '.proto':
                    ret.append(
                        _ProtoInfo.create(scan_relpath, dest_namespace,
                                          relpath))

    return sorted(ret)


def _gather_protos() -> tuple[str, list[tuple[str, str]]]:
    """Gathers all .proto files from the repo and their collective hash.

    Returns Tuple[dgst: str, proto_files: List[Tuple[str, str]]]
      * dgst: The 'overall' checksum for all protos which we ought to have
        installed (as hex).
      * proto_files: a list of source abspath to dest_relpath for these proto
        files (i.e. copy from source to $tmpdir/dest_relpath when constructing
        the to-be-compiled proto tree).

    Raises BadProtoDefinitions if this finds conflicting protos.
    """
    proto_infos = _gather_proto_info()

    csum = hashlib.sha256()

    # If this file changes we need to double-check we can still load all proto
    # files, so add a hash of this file to the checksum.
    csum.update(_file_checksum(__file__).encode())
    csum.update(b'\0')

    csum.update(PROTOC_VERSION)
    csum.update(b'\0')

    rel_to_projs: dict[str, list[str]] = {}
    dups: set[str] = set()
    proto_files: list[tuple[str, str]] = []
    for info in proto_infos:
        duplist = rel_to_projs.setdefault(info.dest_relpath, [])
        duplist.append(info.relpath)
        if len(duplist) > 1:
            dups.add(info.dest_relpath)

        proto_files.append((info.src_abspath, info.dest_relpath))

        csum.update(info.relpath.encode('utf-8'))
        csum.update(b'\0')
        csum.update(info.dest_relpath.encode('utf-8'))
        csum.update(b'\0')
        csum.update(info.blobhash.encode('utf-8'))
        csum.update(b'\0')

    if dups:
        raise BadProtoDefinitions(
            'Multiple .proto files map to the same destination:\n' +
            '\n'.join('  %r from %s' %
                      (relpath, ', '.join(rel_to_projs[relpath]))
                      for relpath in sorted(dups)))

    return csum.hexdigest(), proto_files


class _DirMaker:
    """Makes directories, tolerating existing ones and only making each once."""

    def __init__(self) -> None:
        self.made_dirs: set[str] = set()

    def __call__(self, dirname: str) -> None:
        if dirname in self.made_dirs:
            return
        try:
            os.makedirs(dirname)
        except OSError as ex:
            if ex.errno != errno.EEXIST:
                raise
        toks = dirname.split(os.path.sep)
        curpath = toks[0] + os.path.sep
        self.made_dirs.add(curpath)
        for tok in toks[1:]:
            curpath = os.path.join(curpath, tok)
            self.made_dirs.add(curpath)


def _check_package(modulebody: str, relpath_base: str) -> str | None:
    """Validates the package line of a generated proto module.

    Args:
      * modulebody - The contents of the _pb2.py file.
      * relpath_base - The native-slash-delimited relative path to the
        destination `PB` folder of the generated proto file, minus the '.py'
        extension, e.g. "recipes/brave/subpath".

    Returns None if there's no error, or an error string otherwise.
    """
    parsed = ast.parse(modulebody)
    for assignment in parsed.body:
        if not isinstance(assignment, ast.Assign):
            continue

        assert isinstance(assignment.targets[0], ast.Name)
        if assignment.targets[0].id != 'DESCRIPTOR':
            continue

        # found the file descriptor line
        assert isinstance(assignment.value, ast.Call)
        assert isinstance(assignment.value.args[0], ast.Constant)
        desc = descriptor_pb2.FileDescriptorProto.FromString(
            assignment.value.args[0].value)
        pkg = desc.package
        break
    else:
        return 'unable to find DESCRIPTOR in module'

    relpath_toks = relpath_base.split(os.path.sep)

    is_module_test = lambda toks: (toks[0] == 'recipe_modules' and len(toks) >
                                   3 and toks[3] in ('examples', 'tests'))

    err = None
    toplevel_namespace = relpath_toks[0]
    if toplevel_namespace == 'recipes':
        # Recipes should match the full relpath_base.
        expected = '.'.join(relpath_toks)
        if pkg != expected:
            err = f'expected {expected!r}, got {pkg!r}'

    elif is_module_test(relpath_toks):
        # Recipe module tests should match the full relpath_base.
        expected = '.'.join(relpath_toks)
        if pkg != expected:
            err = f'expected {expected!r}, got {pkg!r}'

    else:
        # Everything else should match the full relpath minus a token.
        expected = '.'.join(relpath_toks[:-1])
        if pkg != expected:
            err = f'expected {expected!r}, got {pkg!r}'

    if err:
        err = f'{relpath_base}.proto: bad package: {err}'

    return err


# We find all import lines which aren't importing from the special
# `google.protobuf` namespace, and rewrite them.
_REWRITE_IMPORT_RE = re.compile(
    r'^from (?!google\.protobuf|typing)(\S*) import (\S*)_pb2 as (.*)$',
    re.MULTILINE)


def _rewrite_and_rename(root: str, base_proto_path: str) -> str | None:
    """Turns a vanilla compiled *_pb2.py file into a recipe proto python file.

    Rewrites the import lines and renames the rewritten *_pb2.py file to just
    *.py.

    Returns None if this was successful, or a string with an error message if
    this failed.
    """
    assert base_proto_path.endswith('_pb2.py'), base_proto_path

    base_pb2 = Path(base_proto_path)
    target = Path(base_proto_path[:-len('_pb2.py')] + '.py')
    content = base_pb2.read_bytes().decode('utf-8')

    # First, process the _pb2.py file: check its package name and rewrite its
    # imports.
    expected_package = os.path.splitext(os.path.relpath(target, root))[0]
    err = _check_package(content, expected_package)

    _write_text(target,
                _REWRITE_IMPORT_RE.sub(r'from PB.\1 import \2 as \3', content))
    base_pb2.unlink()

    # Next, process the .pyi file.
    base_pyi = Path(base_proto_path + 'i')
    pyi_target = Path(base_proto_path[:-len('_pb2.py')] + '.pyi')
    if base_pyi.exists():
        content = base_pyi.read_bytes().decode('utf-8')
        _write_text(
            pyi_target,
            _REWRITE_IMPORT_RE.sub(r'from PB.\1 import \2 as \3', content))
        base_pyi.unlink()

    return err


def _try_rename(src: str, dest: str) -> None:
    """Attempts to os.rename src to dest, swallowing ENOENT errors."""
    try:
        os.rename(src, dest)
    except OSError as exc:
        if exc.errno != errno.ENOENT:
            raise


def _rel_to_abs_replacer(
    proto_files: list[tuple[str, str]], ) -> Callable[[str], str]:
    """Returns a function which will replace directories relative to the
    destination `PB` directory (at the beginning of a line) with their original
    source absolute paths.

    This transforms output like:

        recipe_engine/analyze.proto:7:1: ....

    To:

        /path/to/recipe_engine.git/recipe_engine/analyze.proto:7:1: ....
    """
    rel_to_abs = {}
    for src_abspath, dest_relpath in proto_files:
        dest_dirname = os.path.dirname(dest_relpath)
        if dest_dirname not in rel_to_abs:
            src_base = os.path.dirname(src_abspath)
            if not dest_dirname:  # a root path; needs a slash on replacement
                src_base += os.path.sep
            rel_to_abs[dest_dirname] = src_base

    # Sort all relative paths by length from longest to shortest
    finder = re.compile('^(%s)' % ('|'.join(
        re.escape(rel)
        for rel in sorted(rel_to_abs, key=len, reverse=True)), ))

    # For every match of some relpath, look up the original source path in
    # rel_to_abs and substitute that.
    return lambda to_replace: finder.sub(
        lambda match: rel_to_abs[match.group(0)], to_replace)


def _collect_protos(argfile_fd: int, proto_files: list[tuple[str, str]],
                    dest: str) -> None:
    """Copies all proto_files into dest.

    Writes the list of files to `argfile_fd` which will be passed to protoc.

    Side-effects:
      * Each dest_relpath is written to `argfile_fd` on its own line.
      * Closes `argfile_fd`.
    """
    try:
        _makedirs = _DirMaker()
        for src_abspath, dest_relpath in proto_files:
            destpath = os.path.join(dest, dest_relpath)
            _makedirs(os.path.dirname(destpath))
            shutil.copyfile(src_abspath, destpath)
            os.write(argfile_fd, dest_relpath.encode('utf-8'))
            os.write(argfile_fd, b'\n')
    finally:
        os.close(argfile_fd)  # for windows


def _compile_protos(proto_files: list[tuple[str, str]], proto_tree: str,
                    protoc: str, argfile: str, dest: str) -> None:
    """Runs protoc over the collected protos, renames them and rewrites their
    imports to make them import from `PB`.

    Args:
      * proto_files: Protobuf files.
      * proto_tree: Path to the directory with all the collected .proto
        files.
      * protoc: Path to the protoc binary to use.
      * argfile: Path to a protoc argfile containing a relative path to
        every .proto file in proto_tree on its own line.
      * dest: Path to the destination where the compiled protos should go.
    """
    protoc_proc = subprocess.Popen(
        [protoc, '--python_out', dest, '--pyi_out', dest, '@' + argfile],
        cwd=proto_tree,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT)
    output, _ = protoc_proc.communicate()
    try:
        os.remove(argfile)
    except OSError:
        pass

    if protoc_proc.returncode != 0:
        replacer = _rel_to_abs_replacer(proto_files)
        print('Error while compiling protobufs. Output:\n', file=sys.stderr)
        sys.stderr.write(replacer(output.decode('utf-8')))
        sys.exit(1)

    rewrite_errors = []
    # Walk over all _pb2.py and pyi files (_rewrite_and_rename will process both
    # based on the _pb2.py path)
    for base, _, fnames in os.walk(dest):
        for name in fnames:
            if not name.endswith('_pb2.py'):
                continue
            pb2_path = os.path.join(base, name)
            err = _rewrite_and_rename(dest, pb2_path)
            if err:
                rewrite_errors.append(err)

    if rewrite_errors:
        print('Error while rewriting generated protos. Output:\n',
              file=sys.stderr)
        replacer = _rel_to_abs_replacer(proto_files)
        for error in rewrite_errors:
            print(replacer(error), file=sys.stderr)
        sys.exit(1)


def _install_protos(proto_package_path: str, dgst: str,
                    proto_files: list[tuple[str, str]]) -> None:
    """Installs protos to `{proto_package_path}/PB`.

    Args:
      * proto_package_path - The absolute path to the folder where:
        * We should install protoc as '.../protoc/...'
        * We should install the compiled proto files as '.../PB/...'
        * We should use '.../tmp/...' as a tempdir.
      * dgst - The hexadecimal (lowercase) checksum for the protos we're
        about to install.
      * proto_files: Protobuf files.

    Side-effects:
      * Ensures that `{proto_package_path}/PB` exists and is the correct
        version (checksum).
      * Ensures that `{proto_package_path}/protoc` contains the correct
        `protoc` compiler from CIPD.
    """
    cipd_proc = subprocess.Popen([
        'cipd' + _BAT, 'ensure', '-root',
        os.path.join(proto_package_path, 'protoc'), '-ensure-file', '-'
    ],
                                 stdin=subprocess.PIPE)
    protoc_version = PROTOC_VERSION.split(b'.', 1)[1]
    cipd_proc.communicate(b'infra/3pp/tools/protoc/${platform} version:3@' +
                          protoc_version)
    if cipd_proc.returncode != 0:
        raise ValueError('failed to install protoc: retcode %d' %
                         cipd_proc.returncode)

    # This tmp folder is where all the temporary garbage goes. Future recipe
    # engine invocations will attempt to clean this up as long as PB is
    # up-to-date.
    tmp_base = os.path.join(proto_package_path, 'tmp')

    # proto_tree holds a tree of all the collected .proto files, to be passed to
    # `protoc`
    # pb_temp is the destination of all the generated files; it will be renamed
    # to `{proto_package_path}/dest` as the final step of the installation.
    _DirMaker()(tmp_base)
    proto_tree = tempfile.mkdtemp(dir=tmp_base, prefix='proto_')
    pb_temp = tempfile.mkdtemp(dir=tmp_base, prefix='pb.py_')
    argfile_fd, argfile = tempfile.mkstemp(dir=tmp_base)
    _collect_protos(argfile_fd, proto_files, proto_tree)

    protoc = os.path.join(proto_package_path, 'protoc', 'bin', 'protoc')
    _compile_protos(proto_files, proto_tree, protoc, argfile, pb_temp)
    _write_text(Path(pb_temp, 'csum'), dgst)

    dest = os.path.join(proto_package_path, 'PB')
    # Check the digest again, in case another engine beat us to the punch.
    # This is still racy, but it makes the window substantially smaller.
    if not _check_digest(proto_package_path, dgst):
        old = tempfile.mkdtemp(dir=tmp_base)
        _try_rename(dest, os.path.join(old, 'PB'))
        _try_rename(pb_temp, dest)


def _check_digest(proto_package: str, dgst: str) -> bool:
    """Returns True iff the installed `{proto_package}/PB/csum` matches dgst."""
    try:
        csum_path = Path(proto_package, 'PB', 'csum')
        return csum_path.read_bytes().decode('utf-8') == dgst
    except (OSError, IOError) as exc:
        if exc.errno != errno.ENOENT:
            raise
    return False


@contextlib.contextmanager
def _build_lock(deps_dir: str):
    """Serialize proto builds across processes sharing this cache directory.

    The presubmit runs the recipe unit tests in parallel, so several engine
    processes may call `ensure_compiled` at once against the same
    `.recipe_deps/_pb3`. Without serialization they race on the shared `protoc`
    CIPD root and on the PB compile/rename, corrupting the cache. An exclusive
    lock lets one process build while the rest wait and then reuse the result.

    Uses `fcntl` (POSIX); on platforms without it (Windows) this is a no-op --
    the parallel-build scenario is a POSIX CI concern.
    """
    try:
        import fcntl  # pylint: disable=import-outside-toplevel
    except ImportError:
        yield
        return
    with open(os.path.join(deps_dir, '.build.lock'), 'w') as lock_file:
        fcntl.flock(lock_file, fcntl.LOCK_EX)
        try:
            yield
        finally:
            fcntl.flock(lock_file, fcntl.LOCK_UN)


def ensure_compiled() -> str:
    """Ensures protos are compiled.

    Gathers protos from the repo and compiles them into
    `{RECIPES_ROOT}/.recipe_deps/_pb3/PB/*`.

    Returns the path to the compiled proto package (the parent of `PB`).
    """
    deps_dir = os.path.join(RECIPES_ROOT, '.recipe_deps')
    proto_package = os.path.join(deps_dir, '_pb3')
    _DirMaker()(proto_package)

    dgst, proto_files = _gather_protos()

    # Fast path: the cache is already current, so no lock or build is needed.
    if _check_digest(proto_package, dgst):
        return proto_package

    # Slow path: build under a cross-process lock, re-checking the digest once
    # we hold it (another process may have built it while we waited).
    with _build_lock(deps_dir):
        if not _check_digest(proto_package, dgst):
            try:
                _install_protos(proto_package, dgst, proto_files)
            except Exception:  # pylint: disable=broad-except
                # If some other recipe engine compiled at the same time as us,
                # it may have broken our compilation (e.g. if the other engine
                # cleared tmp out from under us). Double-check the digest to see
                # if it's now what we expect, but raise if not.
                if not _check_digest(proto_package, dgst):
                    raise
        # Clean the tmp scratch dir while holding the lock, so we never delete
        # another process's in-flight build.
        shutil.rmtree(os.path.join(proto_package, 'tmp'), ignore_errors=True)

    return proto_package


def append_to_syspath(proto_package: str) -> None:
    """Append the proto package to sys.path.

    Raises an AssertionError if another package with the same basename is
    already on sys.path.
    """
    for path in sys.path:
        assert os.path.basename(proto_package) != os.path.basename(path), (
            f'{proto_package!r} basename already on sys.path: {path!r}')
    sys.path.append(proto_package)


def is_message_class(obj: object) -> bool:
    """Returns True if |obj| is a subclass of google.protobuf.message.Message."""
    return (inspect.isclass(obj)
            and issubclass(obj, google.protobuf.message.Message))
