# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

# This file is imported in the context of src/PRESUBMIT.py, it uses existing
# functions from a global scope to change them. This allows us to alter root
# Chromium presubmit checks without introducing too much conflict.

import inspect
import os
import re
import traceback

import brave_chromium_utils
import override_utils

# pylint: disable=protected-access,unused-variable

CANNED_CHECKS_KEY = 'canned'


# Helper to load json5 presubmit config.
def load_presubmit_config():
    with brave_chromium_utils.sys_path('//third_party/pyjson5/src'):
        import json5
        return json5.load(
            open(
                brave_chromium_utils.wspath(
                    '//brave/chromium_presubmit_config.json5')))


config = load_presubmit_config()


# Empty check stub that does nothing and returns empty presubmit result.
def noop_check(*_, **__):
    return []


# Replaces existing PRESUBMIT check. Can be used with globals() scope or a class
# scope (such as input_api.canned_checks).
def override_check(scope, name=None, should_exist=True):
    def decorator(new_func):
        is_dict_scope = isinstance(scope, dict)
        check_name = name or new_func.__name__
        if is_dict_scope:
            original_check = scope.get(check_name, None)
        else:
            original_check = getattr(scope, check_name, None)

        if not callable(original_check):
            message = (f'{check_name} check to override not found. '
                       'Please update presubmit overrides.')
            if should_exist:
                traceback.print_stack()
                raise RuntimeError(f'ERROR: {message}')
            print(f'WARNING: {message}')

            return noop_check

        return override_utils.override_function(scope, name=name)(new_func)

    return decorator


# Override canned checks (depot_tools/presubmit_canned_checks.py). These checks
# can be updated with depot_tools (separate from the Chromium revision), so we
# only print a warning instead of failing the presubmits to not break builds.
def override_canned_checks(canned_checks):
    apply_generic_check_overrides(canned_checks, CANNED_CHECKS_KEY, False)

    # Changes from upstream:
    # 1. Run pylint only on *changed* files instead of getting *all* files
    #    from the directory. Upstream does it to catch breakages in
    #    unmodified files, but it's very resource intensive, moreover for
    #    our setup it covers all files from vendor and other directories
    #    which we should ignore.
    @override_check(canned_checks, should_exist=False)
    def GetPylint(original_check, input_api, output_api, **kwargs):
        def _FetchAllFiles(_, input_api, files_to_check, files_to_skip):
            src_filter = lambda f: input_api.FilterSourceFile(
                f, files_to_check=files_to_check, files_to_skip=files_to_skip)
            return [
                f.AbsoluteLocalPath()
                for f in input_api.AffectedSourceFiles(src_filter)
            ]

        with override_utils.override_scope_function(input_api.canned_checks,
                                                    _FetchAllFiles):
            return original_check(input_api, output_api, **kwargs)


# Overrides canned checks and installs per-check file filter.
def modify_input_api(input_api):
    input_api.PRESUBMIT_FIX = os.environ.get('PRESUBMIT_FIX') == '1'
    input_api.DEFAULT_FILES_TO_CHECK += (
        *config['additional_default_files_to_check'], )
    override_canned_checks(input_api.canned_checks)
    setup_per_check_file_filter(input_api)


# Disables checks or forces presubmit errors for checks listed in the config.
def apply_generic_check_overrides(scope, config_key, should_exist):
    for disabled_check in config['disabled_checks'][config_key]:
        override_check(scope, name=disabled_check,
                       should_exist=should_exist)(noop_check)

    def force_presubmit_error_wrapper(original_check, input_api, output_api,
                                      **kwargs):
        with override_utils.override_scope_variable(output_api,
                                                    'PresubmitPromptWarning',
                                                    output_api.PresubmitError):
            return original_check(input_api, output_api, **kwargs)

    for force_error_check in config['checks_to_force_presubmit_errors'][
            config_key]:
        override_check(scope, name=force_error_check,
                       should_exist=should_exist)(force_presubmit_error_wrapper)


# Wraps input_api.change.AffectedFiles method to manually filter files available
# for a current check. The current check is found using the Python stack trace.
def setup_per_check_file_filter(input_api):
    check_function_re = re.compile(config['check_function_regex'])

    def get_check_names(frame):
        check_names = set()
        while frame:
            co_name = frame.f_code.co_name
            if co_name == '_run_check_function':
                break
            if check_function_re.match(co_name):
                check_names.add(co_name)
            frame = frame.f_back
        return check_names

    per_check_files_to_skip = config['per_check_files_to_skip']

    def get_files_to_skip(check_names):
        files_to_skip = [*config['default_files_to_skip']]
        for check_name in check_names:
            files_to_skip.extend(per_check_files_to_skip.get(check_name, []))
        return files_to_skip

    @override_utils.override_method(input_api.change)
    def AffectedFiles(_self, original_method, *args, **kwargs):
        files_to_skip = get_files_to_skip(
            get_check_names(inspect.currentframe().f_back))
        affected_files = input_api.change._affected_files
        if files_to_skip:

            def file_filter(affected_file):
                local_path = affected_file.LocalPath().replace('\\', '/')
                for file_to_skip in files_to_skip:
                    if re.match(file_to_skip, local_path):
                        return False
                return True

            affected_files = [*filter(file_filter, affected_files)]

        with override_utils.override_scope_variable(input_api.change,
                                                    '_affected_files',
                                                    affected_files):
            return original_method(*args, **kwargs)

    if input_api.no_diffs:
        # Modify upstream presubmit runner to check full file content when
        # --files option is passed. This allows us to see all presubmit issues.
        # Upstream uses "no_diffs" mode to speed up checks on all repository,
        # but our codebase is pretty small, so we can run all checks as is.
        input_api.no_diffs = False

        @override_utils.override_method(input_api.change._AFFECTED_FILES)
        def ChangedContents(self, *_, **__):
            if self._cached_changed_contents is not None:
                return self._cached_changed_contents[:]

            result = []
            for line_num, line in enumerate(self.NewContents(), start=1):
                result.append((line_num, line))
            self._cached_changed_contents = result
            return self._cached_changed_contents[:]


# Inlines presubmit file as if it was run from the dir where it's located.
def inline_presubmit(filename, _globals, _locals):
    class State:
        def __init__(self, filename):
            self.presubmit_dir = os.path.dirname(
                brave_chromium_utils.wspath(filename))
            self.orig_cwd = os.getcwd()
            self.orig_presubmit_dir = ''

        def PreRunChecks(self, input_api):
            os.chdir(self.presubmit_dir)
            self.orig_presubmit_dir = input_api._current_presubmit_path
            input_api._current_presubmit_path = self.presubmit_dir

        def PostRunChecks(self, input_api):
            input_api._current_presubmit_path = self.orig_presubmit_dir
            os.chdir(self.orig_cwd)

    state = State(filename)

    def PreRunChecks(input_api, _output_api):
        state.PreRunChecks(input_api)
        return []

    func_suffix = re.sub(r'[^\w]', '_', filename)
    pre_check_name = f'Check_Pre_{func_suffix}'
    assert pre_check_name not in _globals
    _globals[pre_check_name] = PreRunChecks

    brave_chromium_utils.inline_file(filename, _globals, _locals)
    apply_generic_check_overrides(_globals, filename, True)

    def PostRunChecks(input_api, _output_api):
        state.PostRunChecks(input_api)
        return []

    post_check_name = f'Check_Post_{func_suffix}'
    assert post_check_name not in _globals
    _globals[post_check_name] = PostRunChecks
