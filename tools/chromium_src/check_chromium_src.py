#!/usr/bin/env python3
#
# Copyright (c) 2021 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
#
# This script looks for problems in chromium_src.
#
# 1. Look for files in chromium_src that don't have a corresponding
#    file under src tree (or gen).
#
# 2. Look for #defines that redefine symbols that aren't present in
#    the overridden file.
#
# 3. Look for #include statements of the overridden file that have
#    incorrect number of ../ in the path.
#
# !!! This script does return false positives, but better check a
# !!! few of those than not check at all.

import argparse
import re
import os
import sys
import textwrap
import brave_chromium_utils
# Look for potential problems in chromium_src overrides.

BRAVE_SRC = os.path.abspath(
    os.path.dirname(os.path.dirname(os.path.dirname(__file__))))
BRAVE_CHROMIUM_SRC = os.path.join(BRAVE_SRC, 'chromium_src')
CHROMIUM_SRC = os.path.abspath(os.path.dirname(BRAVE_SRC))

# Capture group 1: the name of the macro
# Capture group 2: opening parenthesis if the macro is function-like
POUND_DEFINE_REGEXP = \
    r'^[ \t]*#define[ \t]+([a-zA-Z0-9_]+)[ \t]*(\()?(?:(?:.*\\\r?\n)*.*)'

# Output indent
INDENT = "  "


def strip_comments(content):
    """
    Strips comments from c++ file content
    """
    MULTILINE_COMMENT_REGEX = r'(/\*([\s\S]*?)\*/)'
    ONE_LINE_COMMENT_REGEX = r'(\s*//.*$)'
    stripped = re.sub(MULTILINE_COMMENT_REGEX, '', content, flags=re.MULTILINE)
    return re.sub(ONE_LINE_COMMENT_REGEX, '', stripped, flags=re.MULTILINE)


def is_header_guard_define(override_filepath, target):
    """
    Checks if the target define is the header guard for the override file path
    """
    guard_string = ("BRAVE_CHROMIUM_SRC_" +
                    override_filepath.replace('\\', '/').replace(
                        '/', '_').replace('-', '_').replace('.', '_').upper() +
                    "_")
    return target == guard_string


def get_generated_builddir(build, target_os=None, target_arch=None):
    """
    Returns the path to the generated build directory for a |build|, accounting
    for the optional |target_os| and |target_arch| parameters, if present.
    """
    build_dirname = '_'.join(filter(None, [target_os, build, target_arch]))
    return os.path.join(CHROMIUM_SRC, 'out', build_dirname, 'gen')


def filter_single_chromium_src_filepath(normalized_path, exclude_regexp=None):
    """
    Checks if the path should be filtered out.
    """
    return (exclude_regexp is not None
            and re.search(exclude_regexp, normalized_path) is not None)


def filter_chromium_src_filepaths(affected_paths, exclude_regexp=None):
    """
    Filters affected_paths according to the exclude_regexp.
    """
    result = []
    for path in affected_paths:
        # Strip the BRAVE_CHROMIUM_SRC prefix to match against the
        # list of relative paths to be included and/or excluded
        relative_path = path.replace(BRAVE_CHROMIUM_SRC, '')[1:]

        # Normalize back slashes to forward slashes just in case we're in
        # Windows before trying to match them against a regular expression.
        normalized_path = relative_path.replace('\\', '/')

        should_filter = filter_single_chromium_src_filepath(
            normalized_path, exclude_regexp)
        if not should_filter:
            # Append the OS-dependent relative path instead of the normalized
            # one as that's what functions using this list of paths expect.
            result.append(relative_path)
    return result


def filter_all_chromium_src_filepaths(exclude_regexp):
    """
    Return a list of paths pointing to the files in |BRAVE_CHROMIUM_SRC|
    after filtering out the exclusions.
    """
    result = []
    for dir_path, _dirnames, filenames in os.walk(BRAVE_CHROMIUM_SRC):
        for filename in filenames:
            full_path = os.path.join(dir_path, filename)

            # Strip the BRAVE_CHROMIUM_SRC prefix to match against the
            # list of relative paths to be included and/or excluded
            relative_path = full_path.replace(BRAVE_CHROMIUM_SRC, '')[1:]

            # Normalize back slashes to forward slashes just in case we're in
            # Windows before trying to match them against a regular expression.
            normalized_path = relative_path.replace('\\', '/')

            should_filter = filter_single_chromium_src_filepath(
                normalized_path, exclude_regexp)
            if not should_filter:
                # Append the OS-dependent relative path instead of the
                # normalized one as that's what functions using this list of
                # paths expect.
                result.append(relative_path)
    return result


def is_gen_override(override_filepath):
    """
    Checks if the override path overrides a generated original source based
    on whether it uses a ../gen-prefixed include.
    """
    with open(override_filepath, mode='r', encoding='utf-8') as \
            override_file:
        normalized_override_filepath = override_filepath.replace('\\', '/')
        gen_regexp = r'^#include "\.\./gen/(.*)"'

        for line in override_file:
            line_match = re.search(gen_regexp, line)
            if (line_match
                    and line_match.group(1) == normalized_override_filepath):
                return True
    return False


class ChromiumSrcOverridesChecker:
    def __init__(self, gen_buildir):
        self.gen_buildir = gen_buildir
        self.messages = {'infos': [], 'warnings': [], 'errors': []}
        self.overrides = []
        self.config_data = {}

    def add_notification(self, message_type, message_label, message):
        self.messages[message_type].append(
            "-" * 30 + f"\n[{message_label}] chromium_src:\n" + "\n".join(
                textwrap.wrap(message,
                              initial_indent=INDENT,
                              subsequent_indent=INDENT,
                              break_long_words=False)))

    def add_info(self, message):
        self.add_notification('infos', 'Info', message)

    def add_warning(self, message):
        self.add_notification('warnings', 'Warning', message)

    def add_error(self, message):
        self.add_notification('errors', 'Error', message)

    def do_check_includes(self, override_filepath, original_is_in_gen):
        """
        Checks if |override_filepath| uses relative includes and also checks <>,
        src/ and ../gen-prefixed includes for naming consistency between the
        original and the override.
        """
        with open(override_filepath, mode='r', encoding='utf-8') as \
                override_file:
            normalized_override_filepath = override_filepath.replace('\\', '/')
            display_override_filepath = os.path.join('chromium_src',
                                                     override_filepath)
            override_filename = os.path.basename(override_filepath)
            regexp = r"""
                ^\#include\s
                (?:
                    # Angle brackets include <...> for source files
                    <(.*\.(?:c|cc|cpp|m|mm))>
                    # Angle brackets include <...> for header files followed
                    # by `// IWYU pragma: export`
                    |<(.*\.h)>\s*//\ IWYU\ pragma:\ export
                )
            """
            gen_regexp = r'^#include "\.\./gen/(.*)"'
            rel_regexp = rf'^#include "(\.\./.*{override_filename})"'

            for line in override_file:
                # Check src/-prefixed includes
                line_match = re.search(regexp, line, re.VERBOSE)
                if line_match:
                    line_match_path = line_match.group(1) or line_match.group(
                        2)
                    if original_is_in_gen:
                        self.add_error(
                            f"{display_override_filepath} overrides a " +
                            "generated source file, but does not use a " +
                            "../gen/-prefixed include. A ../gen/-prefixed " +
                            "include should be used instead.")
                    elif line_match_path != normalized_override_filepath:
                        # Check for v8 overrides, they can have includes
                        # starting with src.
                        if normalized_override_filepath.startswith("v8/src"):
                            continue
                        self.add_error(
                            f"{display_override_filepath} uses a <> " +
                            "include that doesn't point to the expected " +
                            f"file. Include: {line}Expected include " +
                            f"target: {normalized_override_filepath}")
                    continue

                # Check ..gen/-prefixed includes
                line_match = re.search(gen_regexp, line)
                if line_match:
                    if not original_is_in_gen:
                        self.add_error(
                            f"{display_override_filepath} is not overriding " +
                            "a generated source file, but uses a " +
                            "../gen/-prefixed include. A src/-prefixed " +
                            "include should be used instead.")
                    elif line_match.group(1) != normalized_override_filepath:
                        self.add_error(
                            f"{display_override_filepath} uses a " +
                            "../gen/-prefixed include that doesn't point to " +
                            f"the expected file. Include: {line}" +
                            "Expected include target: ../gen/" +
                            f"{normalized_override_filepath}")
                    continue

                # Check for relative includes.
                line_match = re.search(rel_regexp, line)
                if not line_match:
                    continue
                self.add_error(
                    f"{display_override_filepath} uses a relative " +
                    f"include: {line}Switch to using a " +
                    f"{'../gen' if original_is_in_gen else 'src'}-prefixed " +
                    "include instead.")

    def validate_define(self, override_filepath, content, target):
        """
        Validates that defined symbol has a corresponding #undef and checks if
        the symbol is used internally in the override file.
        """
        found = {'def_position': -1, 'undef': False, 'other': False}
        pattern = rf'^(.*\b{target}\b.*)$'
        matches = re.finditer(pattern, content, re.MULTILINE)
        for match in matches:
            line = match.group()
            if re.match(rf'^#define[\s\\]+{target}', line):
                # Found #define of the target
                found['def_position'] = match.start()
            elif re.match(rf'^#undef[\s\\]+{target}', line):
                # Found #undef of the target
                found['undef'] = (found['def_position'] >= 0
                                  and match.start() > found['def_position'])
            elif re.fullmatch(r'^[A-Z0-9]+(?:_[A-Z0-9]+)*$', target):
                # Flag as found in the override file.
                # As can be noticed in the regex above, only SHOUTY_CASE
                # defines are considered for this check. This is an incidental
                # assumption that anything named as SHOUTY_CASE is a macro that
                # could either occur in the override file or in the original.
                # When it comes to all the other defines, we assume they are
                # actual references to symbols in the original file.
                # To summarise, a SHOUTY_CASE define appearing in the override
                # file means that it could be consumed internally. A non
                # SHOUTY_CASE define appearing in the override file is
                # expected, and should be ignored, as it still has to appear
                # in the original file.
                found['other'] = True

        display_override_filepath = os.path.join('chromium_src',
                                                 override_filepath)
        if found['def_position'] == -1:
            self.add_error(
                f"SCRIPT ERROR. Expected to find #define {target} in " +
                f"{display_override_filepath}.")
        if not found['undef']:
            message = (f"(MISSING UNDEF) Expected to find #undef {target} " +
                       f"in {display_override_filepath}.")
            if override_filepath.endswith('.h'):
                message += (
                    " If this symbol is intended to propagate beyond this " +
                    "header then place `// CHROMIUM_SRC_NOLINT` comment " +
                    "on the line above the #define.")
            self.add_error(message)

        return found['other']

    def find_marked_defines(self, content, display_override_filepath):
        """
        Finds #define preceeded by ^// CHROMIUM_SRC_INTERNAL_USE$ or
        ^//CHROMIUM_SRC_NOLINT$ above it. Returns a dict of sets of the
        #define names for each comment type.
        """
        INTERNAL_USE_COMMENT_REGEX = re.compile(
            r'^//\sCHROMIUM_SRC_INTERNAL_USE$')
        NO_CHROMIUM_SRC_CHECK_REGEX = re.compile(r'^//\sCHROMIUM_SRC_NOLINT$')
        DEFINE_REGEX = re.compile(r'^\s*#\s*define\s+([A-Za-z_0-9]+)\b')
        marked_defines = {"internal": set(), "nocheck": set()}
        internal_comment_found = False
        nocheck_comment_found = False
        for count, line in enumerate(content.splitlines(), 1):
            if internal_comment_found or nocheck_comment_found:
                match = DEFINE_REGEX.match(line)
                if not match:
                    comment = '// CHROMIUM_SRC_INTERNAL_USE' \
                        if internal_comment_found \
                        else '// CHROMIUM_SRC_NOLINT'
                    self.add_error(
                        f"In {display_override_filepath}:{count}, the " +
                        f"`{comment}` comment is not followed by a #define " +
                        "line.")

                else:
                    if internal_comment_found:
                        marked_defines['internal'].add(match.group(1))
                    else:
                        marked_defines['nocheck'].add(match.group(1))
                internal_comment_found = False
                nocheck_comment_found = False
            else:
                match = INTERNAL_USE_COMMENT_REGEX.match(line)
                if match:
                    internal_comment_found = True
                else:
                    match = NO_CHROMIUM_SRC_CHECK_REGEX.match(line)
                    if match:
                        nocheck_comment_found = True
        return marked_defines

    def do_check_defines(self, override_filepath, original_filepath):
        """
        Finds `#define TARGET REPLACEMENT` statements in |override_filepath| and
        attempts to find the <TARGET> in the |original_filepath|.
        """
        matches = []
        with open(override_filepath, mode='r', encoding='utf-8') as \
                override_file:
            content = override_file.read()
            display_override_filepath = os.path.join('chromium_src',
                                                     override_filepath)
            marked_defines = self.find_marked_defines(
                content, display_override_filepath)
            content = strip_comments(content)

            # Search for all matches for #define. The regex covers:
            # single line, function-like definitions, and multiline defines
            matches = re.findall(POUND_DEFINE_REGEXP,
                                 content,
                                 flags=re.MULTILINE)
            if not matches:
                return 0

            for match in matches:
                target = match[0]

                # Skip header guard defines.
                if (override_filepath.endswith(".h")
                        and is_header_guard_define(override_filepath, target)):
                    continue

                # Skip no-check defines.
                if target in marked_defines['nocheck']:
                    continue

                # Check if the symbol is used internally in the override.
                used_internally = self.validate_define(override_filepath,
                                                       content, target)

                # Adjust target name for BUILDFLAG_INTERNAL_*() cases for
                # function-like matches. match[1] will be set if the
                # definition is function-like.
                if (match[1] and target.startswith('BUILDFLAG_INTERNAL_')):
                    buildflag_match = re.search(r'BUILDFLAG_INTERNAL_(\S*)',
                                                target)
                    target = buildflag_match.group(1)

                # Report ERROR if target can't be found in the original file.
                with open(original_filepath, mode='r', encoding='utf-8') as \
                        original_file:
                    if not re.search(rf"\b{re.escape(target)}\b",
                                     strip_comments(original_file.read())):
                        self.maybe_add_override_symbol_error(
                            marked_defines['internal'], used_internally,
                            target, display_override_filepath,
                            original_filepath)
                    else:
                        if target in marked_defines['internal']:
                            self.add_error(
                                f"Symbol {target} was found in " +
                                f"{original_filepath} but is marked as " +
                                "`// CHROMIUM_SRC_INTERNAL_USE` in the " +
                                f"{display_override_filepath}. Either " +
                                "remove the `// CHROMIUM_SRC_INTERNAL_USE` " +
                                "comment, or change the symbol name to " +
                                "avoid overriding the symbol in the original" +
                                "file.")
        return len(matches)

    def maybe_add_override_symbol_error(self, marked_as_internal_list,
                                        is_used_internally, symbol,
                                        display_override_filepath,
                                        original_filepath):
        if is_used_internally:
            if symbol in marked_as_internal_list:
                return
            self.add_error(
                f"(INTERNAL USE) Symbol {symbol} appears to be used " +
                f"internally in {display_override_filepath}. Symbol is NOT " +
                f"found in {original_filepath}. If this is correct, place " +
                "`// CHROMIUM_SRC_INTERNAL_USE` comment on the line above " +
                "the #define.")
        else:
            self.add_error(
                f"(UNUSED) Override {display_override_filepath} defines " +
                f"symbol {symbol} but the symbol could not be found in " +
                f"{original_filepath} and is not used internally in the " +
                "override. If this is intentional then place " +
                "`// CHROMIUM_SRC_NOLINT` comment on the line above the " +
                "#define.")

    def do_check_overrides(self):
        """
        Checks that each path in the passed in list |overrides_list| exists in
        the passed in directory (|search_dir|), also checks includes.
        """
        count = 0
        for override_filepath in self.overrides:
            original_filepath_found = False
            original_is_in_gen = False
            original_filepath = os.path.join(CHROMIUM_SRC, override_filepath)
            display_override_filepath = os.path.join('chromium_src',
                                                     override_filepath)
            if not os.path.isfile(original_filepath):
                additional_extensions = (
                    brave_chromium_utils.get_additional_extensions())
                if any(
                        override_filepath.endswith(ext)
                        and os.path.isfile(original_filepath.replace(ext, ''))
                        for ext in additional_extensions):
                    original_filepath_found = True
                elif self.gen_buildir is None:
                    # When invoked from presubmit there's no gen_dir, so we can
                    # try to at least check that the include in the override is
                    # consistent with overriding a generated file.
                    if is_gen_override(override_filepath):
                        self.add_warning(
                            f"{display_override_filepath} overrides a " +
                            "generated source file. Existence of the " +
                            "original source and redefined symbols cannot be "
                            + f"verified. Run {os.path.abspath(__file__)} " +
                            "script manually and pass output directory " +
                            "(e.g. Debug) to verify this override.")
                        continue
                else:
                    gen_filepath = os.path.join(self.gen_buildir,
                                                override_filepath)
                    if os.path.isfile(gen_filepath):
                        original_filepath = gen_filepath
                        original_filepath_found = True
                        original_is_in_gen = True
            else:
                original_filepath_found = True
            if not original_filepath_found:
                self.add_error(
                    f"No source for override {display_override_filepath}. " +
                    "If this is not a true override, then add the path to " +
                    "the `path_excludes` in " +
                    "//brave/chromium_src/check_chromium_src_config.json5." +
                    "Otherwise, the upstream file is gone and a fix " +
                    "is required.")
                continue

            count += self.do_check_defines(override_filepath,
                                           original_filepath)
            self.do_check_includes(override_filepath, original_is_in_gen)
        self.add_info(f'Located {count} #define statements.')

    def validate_exclusion_path(self, path):
        """
        Validates that the exclusion file path exists.
        """
        override_path = os.path.join(BRAVE_CHROMIUM_SRC, path)
        if not os.path.isfile(override_path):
            self.add_error(
                "Path listed in " +
                "//brave/chromium_src/check_chromium_src_config.json5 " +
                f"cannot be found: chromium_src/{path}. If the file was " +
                "removed then also remove it from the list in " +
                "//brave/chromium_src/check_chromium_src_config.json5")
            return False
        return True

    def validate_exclusions(self):
        """
        Validate the each path listed in exclusions paths is still a valid path.
        Otherwise the developer who removed the excluded file should also remove
        it from the exclusions list.
        """
        result = True
        for path in self.config_data['path_excludes']:
            if not self.validate_exclusion_path(path):
                result = False

        return result

    def load_exclusions(self):
        config_path = os.path.join(BRAVE_CHROMIUM_SRC,
                                   'check_chromium_src_config.json5')
        if not os.path.isfile(config_path):
            self.add_error(f"Unable to load config file {config_path}.")
            return False

        try:
            json5_path = os.path.join(CHROMIUM_SRC, 'third_party', 'pyjson5',
                                      'src')
            sys.path.append(json5_path)
            import json5

            with open(config_path, "r", encoding='utf-8') as exclusions_file:
                self.config_data = json5.load(exclusions_file)
        finally:
            # Restore sys.path to what it was before.
            sys.path.remove(json5_path)

        keys = ['re_excludes', 'path_excludes']
        for key in keys:
            if not key in self.config_data:
                self.add_error(f"Key '{key}' is missing from {config_path}.")
                return False

        return self.validate_exclusions()

    def check_overrides(self, affected_paths=None):
        # Load and check exclusions
        if not self.load_exclusions():
            return self.messages

        # Change into the chromium_src directory for convenience.
        os.chdir(BRAVE_CHROMIUM_SRC)

        # Build filters
        exclude_regexp = None
        if (len(self.config_data['re_excludes']) > 0
                or len(self.config_data['path_excludes']) > 0):
            exclude_regexp = '|'.join(self.config_data['re_excludes'] +
                                      self.config_data['path_excludes'])

        # Build the list of files to check.
        if affected_paths is None:
            self.overrides = filter_all_chromium_src_filepaths(exclude_regexp)
        else:
            self.overrides = filter_chromium_src_filepaths(
                affected_paths, exclude_regexp)

        # Check overrides
        if len(self.overrides) > 0:
            self.do_check_overrides()

        return self.messages


def main():
    """
    Parse command line args and check all overrides in chromium_src.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('build',
                        help='Build type (i.e. Component|Static|Debug|Release)',
                        nargs='?',
                        default='Component')
    parser.add_argument('--os',
                        required=False,
                        help='Target OS (e.g. android|win|linux|mac|ios)')
    parser.add_argument('--arch',
                        required=False,
                        help='Target architecture (e.g. x86|x64|arm|arm64)')
    options = parser.parse_args()

    gen_buildir = get_generated_builddir(options.build, options.os,
                                         options.arch)

    # Check that the required directories exist.
    for directory in [BRAVE_SRC, gen_buildir]:
        if not os.path.isdir(directory):
            print(f"ERROR: {directory} is not a valid directory.")
            return 1

    messages = ChromiumSrcOverridesChecker(gen_buildir).check_overrides()

    for message in messages['infos']:
        print(message)
    for message in messages['warnings']:
        print(message)
    for message in messages['errors']:
        print(message)
    print(f"\n{len(messages['warnings'])} warning(s) found.")
    num_errors = len(messages['errors'])
    print(f"{num_errors} error(s) found.\n")

    return num_errors


if __name__ == '__main__':
    sys.exit(main())
