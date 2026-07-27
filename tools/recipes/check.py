# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""AST-introspecting check machinery for recipe simulation tests.

A post-process hook receives a `Checker` and calls it in place of `assert`:

    check(cond)          # bare
    check(hint, cond)    # with a human hint

Unlike a plain `assert`, a failing `check(...)` does not raise. Instead the
`Checker` walks the stack, parses the source line of the failing call with
`ast`, and records a `Check` capturing the failing expression *and the resolved
values of its sub-expressions* (the step dict's keys, the missing step name, the
compared values, ...). All checks in a hook run, and `Check.format()` renders
the `CHECK ... (FAIL):` block.

This module carries no dependency on the engine or the step data model: it works
on the plain `{name: step-dict}` mapping the runner builds, so `steps[name]` is a
dict (not a Step object). Failure rendering uses the stdlib `ast.unparse`.
"""

from __future__ import annotations

import ast
import copy
import inspect
import itertools
import re
from collections import OrderedDict, defaultdict, deque, namedtuple
from typing import cast

# Sentinel: distinguishes `check(cond)` from `check(hint, cond)`, and marks
# "attribute/key absent" while walking sub-expressions.
MISSING = object()


def _unparse(node: ast.AST) -> str:
    """Render an AST node back to source.

    Wraps `ast.unparse` (added in Python 3.9) so the no-member warning from
    older pylint releases that predate it stays localized here.
    """
    return ast.unparse(node)  # pylint: disable=no-member


class CheckFrame(namedtuple('CheckFrame', 'fname line function code varmap')):
    """One stack frame relevant to a failed check, rendered for humans."""

    def format(self, indent):
        lines = [
            '%s%s:%s - %s()' %
            ((' ' * indent), self.fname, self.line, self.function)
        ]
        indent += 2
        lines.append('%s`%s`' % ((' ' * indent), self.code))
        indent += 2
        if self.varmap:
            lines.extend('%s%s: %s' % ((' ' * indent), k, v)
                         for k, v in self.varmap.items())
        return lines


class Check(
        namedtuple('Check', ('name ctx_filename ctx_lineno ctx_func ctx_args '
                             'ctx_kwargs frames passed'))):
    """A recorded check outcome plus the frames/values needed to explain it."""

    # filename -> {lineno -> [statements]}
    _PARSED_FILE_CACHE = defaultdict(lambda: defaultdict(list))
    _LAMBDA_CACHE = defaultdict(lambda: defaultdict(list))

    @classmethod
    def create(cls,
               name,
               hook_context,
               frames,
               passed,
               ignore_set,
               additional_varmap=None):
        try:
            keep_frames = [
                cls._process_frame(f, ignore_set, with_vars=False)
                for f in frames[:-1]
            ]
            keep_frames.append(
                cls._process_frame(frames[-1],
                                   ignore_set,
                                   with_vars=True,
                                   additional_varmap=additional_varmap))
        finally:
            # avoid reference cycle as suggested by inspect docs.
            del frames

        return cls(
            name,
            hook_context.filename,
            hook_context.lineno,
            cls._get_name_of_callable(hook_context.func),
            [repr(arg) for arg in hook_context.args],
            {
                k: repr(v)
                for k, v in hook_context.kwargs.items()
            },
            keep_frames,
            passed,
        )

    @classmethod
    def _get_name_of_callable(cls, c):
        if inspect.ismethod(c):
            return c.__self__.__class__.__name__ + '.' + c.__name__
        if inspect.isfunction(c):
            if c.__name__ == (lambda: None).__name__:
                filename = c.__code__.co_filename
                cls._ensure_file_in_cache(filename, c)
                definitions = cls._LAMBDA_CACHE[filename][
                    c.__code__.co_firstlineno]
                assert definitions
                # If there's multiple definitions at the same line, there's not
                # enough information to distinguish which lambda c refers to, so
                # just let python's generic lambda name be used.
                if len(definitions) == 1:
                    return _unparse(definitions[0])
            return c.__name__
        if hasattr(c, '__call__'):
            return c.__class__.__name__ + '.__call__'
        return repr(c)

    @classmethod
    def _get_statements_for_frame(cls, frame):
        raw_frame, filename, lineno, _, _, _ = frame
        cls._ensure_file_in_cache(filename, raw_frame)
        return cls._PARSED_FILE_CACHE[filename][lineno]

    @classmethod
    def _ensure_file_in_cache(cls, filename, obj_with_code):
        """Parse *filename* and index its simple statements by line number.

        Multi-statement nodes (Module, FunctionDef, ...) carry their contents in
        attributes like `body`. The goal is to enqueue those contents and treat
        everything else as a 'single statement' keyed by its line. Cached so
        multiple checks in one file parse it only once. Nested lambdas are also
        indexed so a frame executing a lambda resolves to the lambda's source.
        """
        if filename not in cls._PARSED_FILE_CACHE:
            to_push = ['test', 'body', 'orelse', 'finalbody', 'excepthandler']
            lines, _ = inspect.findsource(obj_with_code)
            queue = deque([ast.parse(''.join(lines), filename)])
            while queue:
                node = queue.pop()
                had_statements = False
                for key in to_push:
                    val = getattr(node, key, MISSING)
                    if val is not MISSING:
                        had_statements = True
                        if isinstance(val, list):
                            # We pop from the start of the queue and want to
                            # append nodes in order, so reverse on extend.
                            queue.extend(val[::-1])
                        else:
                            # 'test' is a single expression, not a list.
                            queue.append(val)
                if had_statements:
                    continue

                real_line = node.lineno
                cls._PARSED_FILE_CACHE[filename][real_line].append(node)

                # Index nested lambdas: a frame executing a lambda that is not on
                # the expression's last line has a different line number than a
                # frame executing the containing expression.
                for n in ast.walk(node):
                    if not isinstance(n, ast.Lambda):
                        continue
                    cls._LAMBDA_CACHE[filename][n.lineno].append(n)
                    lambda_max_line = n.lineno
                    if lambda_max_line != real_line:
                        cls._PARSED_FILE_CACHE[filename][
                            lambda_max_line].append(n)

    @classmethod
    def _process_frame(cls,
                       frame,
                       ignore_set,
                       with_vars,
                       additional_varmap=None):
        """Turn a stack frame into a `CheckFrame`.

        When `with_vars` is set (only the innermost frame), the parsed statement
        is run through `_checkTransformer` to resolve the interesting
        sub-expressions to their runtime values, omitting callables and the step
        mapping itself, and rendering each value with `render_user_value`.
        """
        nodes = cls._get_statements_for_frame(frame)
        raw_frame, filename, lineno, func_name, _, _ = frame

        varmap = None
        if with_vars:
            varmap = dict(additional_varmap or {})

            xfrmr = _checkTransformer(raw_frame.f_locals, raw_frame.f_globals)
            xfrmd = xfrmr.visit(ast.Module(copy.deepcopy(nodes), []))

            for n in itertools.chain(ast.walk(xfrmd), xfrmr.extras):
                if isinstance(n, _resolved):
                    val = n.value
                    if isinstance(val, ast.AST):
                        continue
                    if n.representation in ('True', 'False', 'None'):
                        continue
                    if callable(val) or id(val) in ignore_set:
                        continue
                    if n.representation not in varmap:
                        varmap[n.representation] = render_user_value(val)

        return CheckFrame(filename, lineno, func_name,
                          '; '.join(_unparse(n) for n in nodes), varmap)

    def format(self):
        """Return the lines that make up this check failure.

        Example:
        CHECK "something was run" (FAIL):
          /.../post_process.py:77 - MustRun()
            `check(('step %r must run' % step_name), (step_name in _run_steps))`
              step_name: 'fakiestep'
          added /.../allowlist_steps.py:28
            MustRun('fakiestep')
        """
        ret = [
            'CHECK%(name)s(%(passed)s):' % {
                'name': ' %r ' % self.name if self.name else '',
                'passed': 'PASS' if self.passed else 'FAIL',
            }
        ]
        for frame in self.frames:
            ret.extend(frame.format(indent=2))

        ret.append('  added %s:%d' % (self.ctx_filename, self.ctx_lineno))
        func = '%s(' % self.ctx_func
        if self.ctx_args:
            func += ', '.join(self.ctx_args)
        if self.ctx_kwargs:
            if self.ctx_args:
                func += ', '
            func += ', '.join(['%s=%s' % i for i in self.ctx_kwargs.items()])
        func += ')'
        ret.append('    ' + func)
        return ret


class _resolved(ast.AST):
    """A fake AST node standing in for a resolved sub-expression.

    `_checkTransformer` replaces portions of the parsed statement with these.
    `valid` indicates the value matches the source value, so further source
    operations (attribute access, subscription) may be applied; when False the
    value is a stand-in (e.g. a dict replaced with its keys) and must not be
    operated on further.
    """

    def __init__(self, representation, value, valid=True):
        super().__init__()
        self.representation = representation
        self.value = value
        self.valid = valid


class _checkTransformer(ast.NodeTransformer):
    """Extracts the helpful sub-expressions from a `check(...)` statement.

    Transformations:
      * identifiers resolve to their local/global value;
      * `___ in <dict>` prints `dict.keys()` instead of the whole dict;
      * `a[b][c]` prints `a[b]` and `a[b][c]` (recursively).

    The result is NOT a valid python AST: each reduced sub-expression becomes a
    `_resolved` whose `representation` is the source code and whose `value` is
    the evaluated value. Extra `_resolved` nodes that don't fit the tree land in
    `self.extras`.
    """

    def __init__(self, lvars, gvars):
        self.lvars = lvars
        self.gvars = gvars
        self.extras = []

    @staticmethod
    def _is_valid_resolved(node) -> _resolved | None:
        if isinstance(node, _resolved) and node.valid:
            return node
        return None

    def visit_Compare(self, node: ast.Compare):
        """Match `___ in instanceof(dict)` to print the dict's keys."""
        node = cast(ast.Compare, self.generic_visit(node))

        if len(node.ops) == 1 and isinstance(node.ops[0], (ast.In, ast.NotIn)):
            cmps = node.comparators
            if len(cmps) == 1 and (rslvd := self._is_valid_resolved(cmps[0])):
                if isinstance(rslvd.value, (dict, OrderedDict)):
                    node = ast.Compare(node.left, node.ops, [
                        _resolved(rslvd.representation + '.keys()',
                                  sorted(rslvd.value.keys()),
                                  valid=False)
                    ])

        return node

    def visit_Attribute(self, node: ast.Attribute):
        """Follow attribute access so the resulting value can be printed."""
        node = cast(ast.Attribute, self.generic_visit(node))

        if (rslvd := self._is_valid_resolved(node.value)):
            return _resolved('%s.%s' % (rslvd.representation, node.attr),
                             getattr(rslvd.value, node.attr))

        return node

    def visit_Subscript(self, node: ast.Subscript):
        """Match `__[x]` for constant or resolvable `x`, printing `__[x]`."""
        node = cast(ast.Subscript, self.generic_visit(node))

        node_value_resolved = self._is_valid_resolved(node.value)
        if not node_value_resolved:
            return node

        sliceVal = MISSING
        sliceRepr = ''

        if (rslvd := self._is_valid_resolved(node.slice)):
            # (a[b])[c] -- include `a[b]` in the extras.
            self.extras.append(rslvd)
            sliceVal = rslvd.value
            sliceRepr = rslvd.representation
        elif isinstance(node.slice, ast.Constant):
            sliceVal = node.slice.value
            sliceRepr = repr(sliceVal)

        if sliceVal is not MISSING:
            try:
                return _resolved(
                    '%s[%s]' % (node_value_resolved.representation, sliceRepr),
                    node_value_resolved.value[sliceVal])
            except KeyError:
                if not isinstance(node_value_resolved.value,
                                  (dict, OrderedDict)):
                    raise
                return _resolved(node_value_resolved.representation +
                                 '.keys()',
                                 sorted(node_value_resolved.value.keys()),
                                 valid=False)

        return node

    def visit_Name(self, node):
        """Resolve a bare identifier from constants, then locals, then globals."""
        consts = {'True': True, 'False': False, 'None': None}
        val = consts.get(
            node.id, self.lvars.get(node.id, self.gvars.get(node.id, MISSING)))
        if val is not MISSING:
            return _resolved(node.id, val)
        return node


def render_user_value(val):
    """Render a sub-expression value as an (ideally eval-able) string."""
    if isinstance(val, re.Pattern):
        return render_re(val)
    return repr(val)


def render_re(regex):
    """Render a repr()-style value for a compiled regular expression."""
    actual_flags = []
    if regex.flags:
        flags = [
            (re.IGNORECASE, 'IGNORECASE'),
            (re.LOCALE, 'LOCALE'),
            (re.UNICODE, 'UNICODE'),
            (re.MULTILINE, 'MULTILINE'),
            (re.DOTALL, 'DOTALL'),
            (re.VERBOSE, 'VERBOSE'),
        ]
        for val, name in flags:
            if regex.flags & val:
                actual_flags.append(name)
    if actual_flags:
        return 're.compile(%r, %s)' % (regex.pattern, '|'.join(actual_flags))
    return 're.compile(%r)' % regex.pattern


class Checker:
    """Collects `Check` failures for a single post-process hook.

    Calling the checker records (but does not raise) failures, so a hook reports
    every failed assertion in one pass. The checker MUST be stored in a local
    variable of the caller running the hook: `_call_impl` walks the stack to find
    the frame where the checker is a local, and keeps the frames below it (the
    hook and anything it calls) for the failure backtrace.
    """

    def __init__(self, hook_context, *ignores):
        self.failed_checks: list[Check] = []

        # Objects we never print as sub-expression values. Seed it with the
        # checker itself (printing that has no value).
        self._ignore_set = {id(x) for x in ignores + (self, )}

        self._hook_context = hook_context

    def _call_impl(self, hint, exp):
        if exp:
            return

        # Grab the frames between (non-inclusive) the checker's creation and
        # self.__call__, innermost last.
        try:
            frames = inspect.stack()[2:][::-1]

            # Seed defaults so both names are defined even for an empty stack
            # (i = -1 keeps the whole list; f = None makes the `del` safe).
            i, f = -1, None
            try:
                for i, f in enumerate(frames):
                    # The first frame with self in its locals is where the
                    # checker was created. Use `is` to avoid invoking __eq__.
                    if any(self is obj for obj in f[0].f_locals.values()):
                        break
                frames = frames[i + 1:]
            finally:
                del f

            self.failed_checks.append(
                Check.create(
                    hint,
                    self._hook_context,
                    frames,
                    False,
                    self._ignore_set,
                ))
        finally:
            # avoid reference cycle as suggested by inspect docs.
            del frames

    def __call__(self, arg1, arg2=MISSING):
        if arg2 is not MISSING:
            hint = arg1
            exp = arg2
        else:
            hint = None
            exp = arg1
        self._call_impl(hint, exp)
        return bool(exp)


def VerifySubset(a, b):
    """Verify that `a` is a subset of `b` (both JSON-ish, OrderedDict allowed).

    `a` may introduce no extra dict keys / list elements, must keep the order of
    entries in ordered types, and must keep types consistent. Empty and
    single-element dicts count as subsets of an OrderedDict even though the types
    differ. Returns None when `a` is a valid subset, otherwise a descriptive
    message of what went wrong, e.g.:

      >>> 'object' + VerifySubset({'a': 'thing'}, {'b': 'x', 'a': 'prime'})
      "object['a']: 'thing' != 'prime'"
    """
    if a is b:
        return None

    if isinstance(b, OrderedDict) and isinstance(a, dict):
        # 0 and 1-element dicts can stand in for OrderedDicts.
        if len(a) == 0:
            return None
        if len(a) == 1:
            a = OrderedDict(a)

    if type(a) is not type(b):
        return ': type mismatch: %r v %r' % (type(a).__name__,
                                             type(b).__name__)

    if isinstance(a, OrderedDict):
        last_idx = 0
        b_reverse_index = {k: (i, v) for i, (k, v) in enumerate(b.items())}
        for k, v in a.items():
            j, b_val = b_reverse_index.get(k, (MISSING, MISSING))
            if j is MISSING:
                return ': added key %r' % k

            if j < last_idx:
                return ': key %r is out of order' % k
            # j == last_idx is not possible, these are OrderedDicts
            last_idx = j

            msg = VerifySubset(v, b_val)
            if msg:
                return '[%r]%s' % (k, msg)

    elif isinstance(a, dict):
        for k, v in a.items():
            b_val = b.get(k, MISSING)
            if b_val is MISSING:
                return ': added key %r' % k

            msg = VerifySubset(v, b_val)
            if msg:
                return '[%r]%s' % (k, msg)

    elif isinstance(a, list):
        if len(a) > len(b):
            return ': too long: %d v %d' % (len(a), len(b))

        if not (a or b):
            return None

        bi = ai = 0
        while bi < len(b) - 1 and ai < len(a) - 1:
            msg = VerifySubset(a[ai], b[bi])
            if msg is None:
                ai += 1
            bi += 1
        if ai != len(a) - 1:
            return ': added %d elements' % (len(a) - 1 - ai)

    elif isinstance(a, (str, int, bool, type(None))):
        if a != b:
            return ': %r != %r' % (a, b)

    else:
        return ': unknown type: %r' % (type(a).__name__)

    return None


class PostProcessError(ValueError):
    """Raised when a post-process hook returns a non-subset expectation."""
