# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import collections.abc
import copy
import os
import sys

import brave_chromium_utils
import brave_node
import chromium_presubmit_overrides
import override_utils

PRESUBMIT_VERSION = '2.0.0'


# Adds support for chromium_presubmit_config.json5 and some helpers.
def CheckToModifyInputApi(input_api, _output_api):
    chromium_presubmit_overrides.modify_input_api(input_api)
    return []


# Check Leo variables actually exist
def CheckLeoVariables(input_api, output_api):
    def _web_files_filter(affected_file):
        return input_api.FilterSourceFile(
            affected_file,
            files_to_check=[
                r'.+\.(js|jsx|ts|tsx|css|less|lss|sass|scss|svelte)$',
                r'package\.json$'
            ])

    # If no web files were affected, this shouldn't change any Leo variables, so
    # we can skip running leo-check.
    if not any(
            input_api.AffectedFiles(file_filter=_web_files_filter,
                                    include_deletes=False)):
        return []

    try:
        parts = [
            brave_node.PathInNodeModules('@brave', 'leo', 'src', 'scripts',
                                         'audit-tokens.js')
        ]
        brave_node.RunNode(parts)
        return []
    except RuntimeError as err:
        return [output_api.PresubmitError(err.args[1])]

# Check and fix formatting issues (supports --fix).
def CheckPatchFormatted(input_api, output_api):
    # Use git cl format to format supported files with Chromium formatters.
    git_cl_format_cmd = [
        '-C',
        input_api.change.RepositoryRoot(),
        'cl',
        'format',
        '--presubmit',
    ]

    # Keep in sync with `npm run format` command.
    git_cl_format_cmd.extend([
        '--python',
        '--no-rust-fmt',
    ])

    # Make sure the passed --upstream branch is applied to git cl format.
    if input_api.change.UpstreamBranch():
        git_cl_format_cmd.extend(
            ['--upstream', input_api.change.UpstreamBranch()])

    # Do a dry run if --fix was not passed.
    if not input_api.PRESUBMIT_FIX:
        git_cl_format_cmd.append('--dry-run')

    # Pass a path where the current PRESUBMIT.py file is located.
    git_cl_format_cmd.append(input_api.PresubmitLocalPath())

    with brave_chromium_utils.sys_path("//brave/vendor/depot_tools"):
        import git_cl

    # Run git cl format and get return code.
    git_cl_format_code, _ = git_cl.RunGitWithCode(git_cl_format_cmd)
    if git_cl_format_code not in (0, 2):
        return [
            output_api.PresubmitError(
                f'Presubmit format check has failed, return code: {git_cl_format_code}'
            )
        ]

    is_format_required = git_cl_format_code == 2

    if not is_format_required or input_api.PRESUBMIT_FIX:
        # Use Prettier to format other file types.
        files_to_check = (
            # Enable when files will be formatted.
            # r'.+\.js$',
            # r'.+\.ts$',
            # r'.+\.tsx$',
        )
        files_to_skip = input_api.DEFAULT_FILES_TO_SKIP

        file_filter = lambda f: input_api.FilterSourceFile(
            f, files_to_check=files_to_check, files_to_skip=files_to_skip)
        affected_files = input_api.AffectedFiles(file_filter=file_filter,
                                                 include_deletes=False)
        files_to_format = [f.AbsoluteLocalPath() for f in affected_files]

        node_args = [
            brave_node.PathInNodeModules('prettier', 'bin-prettier'),
            '--write' if input_api.PRESUBMIT_FIX else '--check',
        ]

        files_per_command = 25 if input_api.is_windows else 1000
        for i in range(0, len(files_to_format), files_per_command):
            args = node_args + files_to_format[i:i + files_per_command]
            try:
                brave_node.RunNode(args)
            except RuntimeError as err:
                if 'Forgot to run Prettier?' in str(err):
                    is_format_required = True
                    break
                # Raise on unexpected output. Could be node or prettier issues.
                raise

    if is_format_required:
        if input_api.PRESUBMIT_FIX:
            raise RuntimeError('--fix was passed, but format has failed')
        short_path = input_api.basename(input_api.change.RepositoryRoot())
        git_cl_format_cmd.remove('--dry-run')
        git_cl_format_cmd.append('--diff')
        _, diff_output = git_cl.RunGitWithCode(git_cl_format_cmd)
        return [
            output_api.PresubmitError(
                f'The {short_path} directory requires source formatting. '
                f'Please run: npm run presubmit -- --fix.\n\n{diff_output}')
        ]
    return []


# Check and fix ESLint issues (supports --fix).
def CheckESLint(input_api, output_api):
    files_to_check = (
        r'.+\.js$',
        r'.+\.ts$',
        r'.+\.tsx$',
    )
    files_to_skip = input_api.DEFAULT_FILES_TO_SKIP

    file_filter = lambda f: input_api.FilterSourceFile(
        f, files_to_check=files_to_check, files_to_skip=files_to_skip)
    files_to_check = input_api.AffectedFiles(file_filter=file_filter,
                                             include_deletes=False)

    with brave_chromium_utils.sys_path('//tools'):
        from web_dev_style import js_checker
        return js_checker.JSChecker(input_api,
                                    output_api).RunEsLintChecks(files_to_check)


def CheckWebDevStyle(input_api, output_api):
    with brave_chromium_utils.sys_path('//tools'):
        from web_dev_style import presubmit_support, js_checker
        # Disable RunEsLintChecks, it's run separately in CheckESLint.
        with override_utils.override_scope_function(
                js_checker.JSChecker,
                chromium_presubmit_overrides.noop_check,
                name='RunEsLintChecks'):
            return presubmit_support.CheckStyle(input_api, output_api)


def CheckChangeLintsClean(input_api, output_api):
    return input_api.canned_checks.CheckChangeLintsClean(input_api,
                                                         output_api,
                                                         lint_filters=[])


def CheckPylint(input_api, output_api):
    extra_paths_list = os.environ['PYTHONPATH'].split(os.pathsep)
    disabled_warnings = [
        'import-outside-toplevel',
        'line-too-long',
    ]
    return input_api.canned_checks.RunPylint(
        input_api,
        output_api,
        extra_paths_list=extra_paths_list,
        disabled_warnings=disabled_warnings)


def CheckLicense(input_api, output_api):
    """Verifies the Brave license header."""

    files_to_check = input_api.DEFAULT_FILES_TO_CHECK + (r'.+\.gni?$', )
    files_to_skip = input_api.DEFAULT_FILES_TO_SKIP + (
        r"\.storybook/",
        r"ios/browser/api/brave_rewards/legacy_database/core_data_models/",
        r'win_build_output/',
    )

    current_year = int(input_api.time.strftime('%Y'))
    allowed_years = (str(s) for s in reversed(range(2015, current_year + 1)))
    years_re = '(' + '|'.join(allowed_years) + ')'

    # License regexp to match in NEW and MOVED files, it doesn't allow variance.
    # Note: presubmit machinery cannot distinguish between NEW and MOVED files,
    # that's why we cannot force this regexp to have a precise year, also
    # uplifts may fail during year change period, so the year check is relaxed.
    new_file_license_re = input_api.re.compile((
        r'.*? Copyright \(c\) %(year)s The Brave Authors\. All rights reserved\.\n'
        r'.*? This Source Code Form is subject to the terms of the Mozilla Public\n'
        r'.*? License, v\. 2\.0\. If a copy of the MPL was not distributed with this file,\n'
        r'.*? You can obtain one at https://mozilla.org/MPL/2\.0/\.(?: \*/)?\n')
                                               % {'year': years_re},
                                               input_api.re.MULTILINE)

    # License regexp to match in EXISTING files, it allows some variance.
    existing_file_license_re = input_api.re.compile((
        r'.*? Copyright \(c\) %(year)s The Brave Authors\. All rights reserved\.\n'
        r'.*? This Source Code Form is subject to the terms of the Mozilla Public\n'
        r'.*? License, v\. 2\.0\. If a copy of the MPL was not distributed with this(\n.*?)? file,\n?'
        r'.*? (y|Y)ou can obtain one at https?://mozilla.org/MPL/2\.0/\.(?: \*/)?\n'
    ) % {'year': years_re}, input_api.re.MULTILINE)

    # License template for new files. Includes current year.
    expected_license_template = (
        '%(comment)s Copyright (c) %(year)s The Brave Authors. All rights reserved.\n'
        '%(comment)s This Source Code Form is subject to the terms of the Mozilla Public\n'
        '%(comment)s License, v. 2.0. If a copy of the MPL was not distributed with this file,\n'
        '%(comment)s You can obtain one at https://mozilla.org/MPL/2.0/.\n') % {
            'comment': '#',
            'year': current_year,
        }

    bad_new_files = []
    bad_files = []
    sources = lambda affected_file: input_api.FilterSourceFile(
        affected_file,
        files_to_check=files_to_check,
        files_to_skip=files_to_skip)
    for f in input_api.AffectedSourceFiles(sources):
        contents = input_api.ReadFile(f, 'r')[:1000].replace('\r\n', '\n')
        if not contents:
            continue
        if f.Action() == 'A':  # 'A' means "Added", also includes moved files.
            if not new_file_license_re.search(contents):
                bad_new_files.append(f.LocalPath())
        else:
            if not existing_file_license_re.search(contents):
                bad_files.append(f.LocalPath())

    splitted_expected_license_template = expected_license_template.replace(
        "# ", "").split('\n')
    multiline_comment_expected_license = (
        f'/* {splitted_expected_license_template[0]}\n'
        f' * {splitted_expected_license_template[1]}\n'
        f' * {splitted_expected_license_template[2]}\n'
        f' * {splitted_expected_license_template[3]} */\n')
    assert new_file_license_re.search(expected_license_template)
    assert existing_file_license_re.search(expected_license_template)
    assert new_file_license_re.search(multiline_comment_expected_license)
    assert existing_file_license_re.search(multiline_comment_expected_license)

    # Show this to simplify copy-paste when an invalid license is found.
    expected_licenses = (f'{expected_license_template.replace("#", "//")}\n'
                         f'{multiline_comment_expected_license}\n'
                         f'{expected_license_template}')

    result = []
    if bad_new_files:
        expected_license_message = (
            f'Expected one of license headers in new files:\n'
            f'{expected_licenses}')
        result.append(
            output_api.PresubmitError(expected_license_message,
                                      items=bad_new_files))
    if bad_files:
        expected_license_message = (
            f'Expected one of license headers in existing files:\n'
            f'{expected_licenses.replace(f"{current_year}", "<year>")}')
        result.append(
            output_api.PresubmitPromptWarning(expected_license_message,
                                              items=bad_files))
    return result


def CheckNewSourceFileWithoutGnChangeOnUpload(input_api, output_api):
    """Checks newly added source files have corresponding GN changes."""
    files_to_skip = input_api.DEFAULT_FILES_TO_SKIP + (r"chromium_src/.*", )

    source_file_filter = lambda f: input_api.FilterSourceFile(
        f,
        files_to_check=(r'.+\.cc$', r'.+\.c$', r'.+\.mm$', r'.+\.m$'),
        files_to_skip=files_to_skip)

    new_sources = []
    for f in input_api.AffectedSourceFiles(source_file_filter):
        if f.Action() != 'A':
            continue
        new_sources.append(f.LocalPath())

    gn_file_filter = lambda f: input_api.FilterSourceFile(
        f,
        files_to_check=(r'.+\.gn$', r'.+\.gni$'),
        files_to_skip=files_to_skip)

    all_gn_changed_contents = ''
    for f in input_api.AffectedSourceFiles(gn_file_filter):
        for _, line in f.ChangedContents():
            all_gn_changed_contents += line

    problems = []
    for source in new_sources:
        basename = input_api.os_path.basename(source)
        if basename not in all_gn_changed_contents:
            problems.append(source)

    if problems:
        return [
            output_api.PresubmitError(
                'Missing GN changes for new .cc/.c/.mm/.m source files',
                items=sorted(problems),
                long_text=
                'Please double check whether newly added source files need '
                'corresponding changes in gn or gni files.')
        ]
    return []

# DON'T ADD NEW BRAVE CHECKS AFTER THIS LINE.
#
# This call inlines Chromium checks into current scope from src/PRESUBMIT.py. We
# do this to have the right order of checks, so all `--fix`-aware checks are
# executed first.
chromium_presubmit_overrides.inline_presubmit('//PRESUBMIT.py', globals(),
                                              locals())

# pyright: reportUnboundVariable=false, reportUndefinedVariable=false

_BANNED_CPP_FUNCTIONS += (
    BanRule(
        r'/\b(Basic|W)?StringPiece(16)?\b',
        ('Use std::string_view instead', ),
        True,
        [_THIRD_PARTY_EXCEPT_BLINK],  # Don't warn in third_party folders.
    ),
    BanRule(
        'base::PathService::Get',
        ('Prefer using base::PathService::CheckedGet() instead', ),
        treat_as_error=False,
        excluded_paths=[_THIRD_PARTY_EXCEPT_BLINK],
    ),
    BanRule(
        r'/\bEnableStackLogging\(\)',
        ('Do not commit EnableStackLogging() call, it\'s not intended for production use',
         ),
        treat_as_error=True,
        excluded_paths=[_THIRD_PARTY_EXCEPT_BLINK],
    ),
)


# Extend BanRule exclude lists with Brave-specific paths.
def ApplyBanRuleExcludes():
    # Collect all _BANNED_* variables declared in //PRESUBMIT.py.
    ban_rule_lists = [
        value for name, value in globals().items()
        if name.startswith('_BANNED_')
        and isinstance(value, collections.abc.Sequence) and len(value) > 0
        and isinstance(value[0], BanRule)
    ]

    # Get additional excluded paths from the config.
    ban_rule_excluded_paths = chromium_presubmit_overrides.config.get(
        'ban_rule_excluded_paths')

    # Add excluded paths to BanRule instances.
    all_patterns = {*ban_rule_excluded_paths.keys()}
    used_patterns = set()
    for ban_rule_list in ban_rule_lists:
        for ban_rule in ban_rule_list:
            excluded_paths = ban_rule_excluded_paths.get(ban_rule.pattern)
            if not excluded_paths:
                continue

            used_patterns.add(ban_rule.pattern)
            if ban_rule.excluded_paths is None:
                ban_rule.excluded_paths = (*excluded_paths, )
            else:
                ban_rule.excluded_paths += (*excluded_paths, )

    # Fail if some pattern was not used.
    unused_patterns = all_patterns - used_patterns
    if unused_patterns:
        raise RuntimeError(f'ERROR: Unused ban_rule_excluded_paths patterns: '
                           f'{unused_patterns}')


ApplyBanRuleExcludes()


@chromium_presubmit_overrides.override_check(globals())
def CheckForIncludeGuards(original_check, input_api, output_api, **kwargs):
    # Add 'brave/' prefix for header guard checks to properly validate guards.
    def AffectedSourceFiles(self, original_method, source_file):
        def PrependBrave(affected_file):
            affected_file = copy.copy(affected_file)
            affected_file._path = f'brave/{affected_file._path}'
            return affected_file

        return [
            PrependBrave(f)
            for f in filter(self.FilterSourceFile, original_method(source_file))
        ]

    with override_utils.override_scope_function(input_api, AffectedSourceFiles):
        return original_check(input_api, output_api, **kwargs)


# Use BanRule.excluded_paths in all BanRule-like checks.
@override_utils.override_function(globals())
def _GetMessageForMatchingType(orig, input_api, f, line_num, line, ban_rule):
    def IsExcludedFile(affected_file, excluded_paths):
        if not excluded_paths:
            return False

        local_path = affected_file.LocalPath()
        # Consistently use / as path separator to simplify the writing of regex
        # expressions.
        local_path = local_path.replace(input_api.os_path.sep, '/')
        for item in excluded_paths:
            if input_api.re.match(item, local_path):
                return True
        return False

    if IsExcludedFile(f, ban_rule.excluded_paths):
        return []

    return orig(input_api, f, line_num, line, ban_rule)


@override_utils.override_function(globals())
def _ChangeHasSecurityReviewer(*_):
    # We don't have Gerrit API available to check for reviewers.
    return False


@chromium_presubmit_overrides.override_check(globals())
def CheckJavaStyle(_original_check, input_api, output_api):
    """ Copy of upstream's CheckJavaStyle. The only difference - it uses
    brave/tools/android/checkstyle/brave-style-5.0.xml style file where all
    errors are replaced with warnings except UnusedImports.
    When all style error will be fixed, this function should be removed and
    the original function from upstream must be used again """
    def _IsJavaFile(input_api, file_path):
        return input_api.os_path.splitext(file_path)[1] == ".java"

    # Return early if no java files were modified.
    if not any(
            _IsJavaFile(input_api, f.LocalPath())
            for f in input_api.AffectedFiles()):
        return []

    # Android toolchain is only available on Linux.
    if not sys.platform.startswith('linux'):
        return []

    with brave_chromium_utils.sys_path('//tools/android/checkstyle'):
        import checkstyle

    files_to_skip = input_api.DEFAULT_FILES_TO_SKIP

    # Filter out non-Java files and files that were deleted.
    java_files = [
        x.AbsoluteLocalPath() for x in
        input_api.AffectedSourceFiles(lambda f: input_api.FilterSourceFile(
            f, files_to_skip=files_to_skip)) if x.LocalPath().endswith('.java')
    ]
    if not java_files:
        return []

    local_path = os.path.join(input_api.PresubmitLocalPath(), 'brave')
    style_file = os.path.join(input_api.PresubmitLocalPath(), 'brave', 'tools',
                              'android', 'checkstyle', 'brave-style-5.0.xml')
    violations = checkstyle.run_checkstyle(local_path, style_file, java_files)
    warnings = ['  ' + str(v) for v in violations if v.is_warning()]
    errors = ['  ' + str(v) for v in violations if v.is_error()]

    ret = []
    if warnings:
        ret.append(output_api.PresubmitPromptWarning('\n'.join(warnings)))
    if errors:
        msg = '\n'.join(errors)
        if 'Unused import:' in msg or 'Duplicate import' in msg:
            msg += """

To remove unused imports: ./tools/android/checkstyle/remove_unused_imports.sh"""
        ret.append(output_api.PresubmitError(msg))
    return ret

# DON'T ADD NEW CHECKS HERE, ADD THEM BEFORE FIRST inline_presubmit().
