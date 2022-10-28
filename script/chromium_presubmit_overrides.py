# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

# This file is imported in the context of src/PRESUBMIT.py, it uses existing
# functions from a global scope to change them. This allows us to alter root
# Chromium presubmit checks without introducing too much conflict.

import copy
import inspect
import re
import sys

import override_utils
import import_inline

# pylint: disable=line-too-long,protected-access,unused-variable

GLOBAL_CHECKS_KEY = 'global'
CANNED_CHECKS_KEY = 'canned'


# Helper to load json5 presubmit config.
def load_presubmit_config():
    try:
        json5_path = import_inline.join_src_dir('third_party', 'pyjson5',
                                                'src')
        sys.path.append(json5_path)
        # pylint: disable=import-outside-toplevel,import-error
        import json5
        return json5.load(
            open(
                import_inline.join_src_dir('brave',
                                           'chromium_presubmit_config.json5')))
    finally:
        # Restore sys.path to what it was before.
        sys.path.remove(json5_path)


config = load_presubmit_config()


# Empty check stub that does nothing and returns empty presubmit result.
def noop_check(*_, **__):
    return []


# Replaces existing PRESUBMIT check. Can be used with globals() scope or a class
# scope (such as input_api.canned_checks). Doesn't fail if the Check is not
# found, only prints a warning message."""
def override_check(scope, name=None):
    def decorator(new_func):
        is_dict_scope = isinstance(scope, dict)
        check_name = name or new_func.__name__
        if is_dict_scope:
            original_check = scope.get(check_name, None)
        else:
            original_check = getattr(scope, check_name, None)

        if not callable(original_check):
            print(f'WARNING: {check_name} check to override not found.\n'
                  'Please update chromium_presubmit_overrides.py!')

            return noop_check

        return override_utils.override_function(scope, name=name)(new_func)

    return decorator


# Returns first Check* method from the scope.
def get_first_check_name(scope):
    assert isinstance(scope, dict)
    for key, value in scope.items():
        if key.startswith('Check') and callable(value):
            return key
    raise LookupError('Check* method not found in scope')


# Override src/PRESUBMIT.py checks.
def override_global_checks(global_checks):
    apply_generic_check_overrides(global_checks, GLOBAL_CHECKS_KEY)

    # Changes from upstream:
    # 1. Add 'brave/' prefix for header guard checks to properly validate
    #    guards.
    @override_check(global_checks)
    def CheckForIncludeGuards(original_check, input_api, output_api, **kwargs):
        def AffectedSourceFiles(self, original_method, source_file):
            def PrependBrave(affected_file):
                affected_file = copy.copy(affected_file)
                affected_file._path = f'brave/{affected_file._path}'
                return affected_file

            return [
                PrependBrave(f) for f in filter(self.FilterSourceFile,
                                                original_method(source_file))
            ]

        with override_utils.override_scope_function(input_api,
                                                    AffectedSourceFiles):
            return original_check(input_api, output_api, **kwargs)

    # Changes from upstream:
    # 1. Remove ^(chrome|components|content|extensions) filter to cover all
    #    files in the repository, because brave/ structure is slightly
    #    different.
    @override_check(global_checks)
    def CheckMPArchApiUsage(original_check, input_api, output_api, **kwargs):
        def AffectedFiles(self, original_method, *args, **kwargs):
            kwargs['file_filter'] = self.FilterSourceFile
            return original_method(*args, **kwargs)

        with override_utils.override_scope_function(input_api, AffectedFiles):
            return original_check(input_api, output_api, **kwargs)


# Override canned checks (depot_tools/presubmit_canned_checks.py).
def override_canned_checks(canned_checks):
    apply_generic_check_overrides(canned_checks, CANNED_CHECKS_KEY)

    # Changes from upstream:
    # 1. Replace suggested command from upstream-specific to 'npm run format'.
    @override_check(canned_checks)
    def CheckPatchFormatted(original_check, input_api, output_api, **kwargs):
        kwargs = {
            **kwargs,
            'bypass_warnings': False,
            'check_python': True,
        }
        result = original_check(input_api, output_api, **kwargs)
        # If presubmit generates "Please run git cl format --js" message, we
        # should replace the command with "npm run format -- --js". The
        # order of these replacements ensure we do this properly.
        replacements = [
            (' format --', ' format -- --'),
            ('git cl format', 'npm run format'),
            ('gn format', 'npm run format'),
            ('rust-fmt', 'rust'),
            ('swift-format', 'swift'),
        ]
        for item in result:
            for replacement in replacements:
                item._message = item._message.replace(replacement[0],
                                                      replacement[1])
        return result

    # Changes from upstream:
    # 1. Run pylint only on *changed* files instead of getting *all* files
    #    from the directory. Upstream does it to catch breakages in
    #    unmodified files, but it's very resource intensive, moreover for
    #    our setup it covers all files from vendor and other directories
    #    which we should ignore.
    @override_check(canned_checks)
    def GetPylint(original_check, input_api, output_api, **kwargs):
        def _FetchAllFiles(_, input_api, files_to_check, files_to_skip):
            src_filter = lambda f: input_api.FilterSourceFile(
                f, files_to_check=files_to_check, files_to_skip=files_to_skip)
            return [
                f.LocalPath()
                for f in input_api.AffectedSourceFiles(src_filter)
            ]

        with override_utils.override_scope_function(input_api.canned_checks,
                                                    _FetchAllFiles):
            return original_check(input_api, output_api, **kwargs)


# Overrides canned checks and installs per-check file filter.
def modify_input_api(input_api):
    override_canned_checks(input_api.canned_checks)
    setup_per_check_file_filter(input_api)
    input_api.DEFAULT_FILES_TO_SKIP += (*config['default_files_to_skip'], )


# Disables checks or forces presubmit errors for checks listed in the config.
def apply_generic_check_overrides(scope, config_key):
    for disabled_check in config['disabled_checks'][config_key]:
        override_check(scope, name=disabled_check)(noop_check)

    def force_presubmit_error_wrapper(original_check, input_api, output_api,
                                      **kwargs):
        with override_utils.override_scope_variable(output_api,
                                                    'PresubmitPromptWarning',
                                                    output_api.PresubmitError):
            return original_check(input_api, output_api, **kwargs)

    for force_error_check in config['checks_to_force_presubmit_errors'][
            config_key]:
        override_check(scope,
                       name=force_error_check)(force_presubmit_error_wrapper)


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
        files_to_skip = []
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


def Apply(_globals):
    override_global_checks(_globals)

    # Override the first global check to modify input_api before any check is
    # run.
    @override_check(_globals, get_first_check_name(_globals))
    def OverriddenFirstCheck(original_check, input_api, output_api, **kwargs):
        modify_input_api(input_api)
        return original_check(input_api, output_api, **kwargs)
