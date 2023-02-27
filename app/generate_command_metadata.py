# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import optparse
from os import path

# Prefix indicating the line is a command
COMMAND_PREFIX = "#define IDC_"

# Any lines with one of these prefixes is important - we want to keep command
# #ifdefs to ensure they only show up on platforms they're actually available on
PREFIXES_TO_KEEP = [COMMAND_PREFIX, "#if", "#elif", "#else", "#endif"]

# Commands which shouldn't be included, as they either don't work, crash, or
# aren't acceleratable.
EXCLUDE_COMMANDS = [
    "IDC_MinimumLabelValue",
    # All spellcheck commands are excluded, they only work from right click menu
    "IDC_SPELLCHECK",
    "IDC_CHECK_SPELLING_WHILE_TYPING",
    "IDC_SPELLPANEL_TOGGLE",

    # Writing direction commands only work from context menu
    "IDC_WRITING_DIRECTION",

    # These are a range, not actual commands
    "IDC_CONTENT_CONTEXT_CUSTOM_FIRST",
    "IDC_EXTENSIONS_CONTEXT_CUSTOM_LAST",

    # These content context commands only work from a context menu
    "_CONTEXT_",

    # Requires a current url
    "IDC_OPEN_CURRENT_URL",

    # Not acceleratable - crashes when the conditions aren't met.
    "IDC_MANAGE_PASSWORDS_FOR_PAGE",

    # Not supported in Brave
    "IDC_SHOW_SIGNIN",
    "IDC_FOLLOW",
    "IDC_UNFOLLOW",
    "IDC_VIRTUAL_CARD_ENROLL",
    "IDC_VIRTUAL_CARD_MANUAL_FALLBACK",
    "IDC_RUN_SCREEN_AI_VISUAL_ANNOTATIONS",

    # Crashes if speedreader doesn't work on page.
    "IDC_SPEEDREADER_ICON_ONCLICK",

    # Not useful menu commands - mostly these are submenu commands, which do
    # nothing unless the parent menu is open.
    "IDC_APP_MENU_IPFS",
    "IDC_APP_MENU_IPFS_PUBLISH_LOCAL_FILE",
    "IDC_APP_MENU_IPFS_PUBLISH_LOCAL_FOLDER",
    "IDC_APP_MENU_IPFS_SHARE_LOCAL_FILE",
    "IDC_APP_MENU_IPFS_SHARE_LOCAL_FOLDER"
    "IDC_APP_MENU_IPFS_OPEN_FILES",
    "IDC_APP_MENU_IPFS_UPDATE_IPNS",
    "IDC_ZOOM_MENU",
    "IDC_DEVELOPER_MENU",
    "IDC_BOOKMARKS_MENU",
    "IDC_RECENT_TABS_MENU",
    "IDC_HELP_MENU",
    "IDC_SIDEBAR_SHOW_OPTION_MENU",
    "IDC_BRAVE_BOOKMARK_BAR_SUBMENU",
    "IDC_DEBUG_FRAME_TOGGLE",
    "IDC_SEND_TAB_TO_SELF",

    # Not actually commands
    "IDC_BRAVE_COMMANDS_START",
    "IDC_BRAVE_COMMANDS_LAST"
]


def extract_relevant_lines(filename):
    """Returns all lines in a file which affect what commands are available"""
    guard_name_includes = path.basename(filename) \
      .upper() \
      .replace('/', '_') \
      .replace('.', '_') + '_'

    def is_relevant(line):
        return guard_name_includes not in line \
          and any(line.startswith(prefix) for prefix in PREFIXES_TO_KEEP) \
          and not any(command in line for command in EXCLUDE_COMMANDS)

    with open(filename) as f:
        return [line.strip() for line in f if is_relevant(line)]


def generate_command_info(command_definition_files, template_file):
    """Generates command_utils.cc with definitions for GetCommands and
       GetCommandName"""
    lines = []
    for command_definition_file in command_definition_files:
        lines += extract_relevant_lines(command_definition_file)

    result = ''
    with open(template_file) as f:
        result = f.read()

    def get_line(line):
        if line.startswith(COMMAND_PREFIX):
            command_name = line.split(' ')[1][4:]
            return f'        ADD_UNTRANSLATED_COMMAND({command_name}),'

        return line

    command_definitions = map(get_line, lines)
    return result.replace('TEMPLATE_PLACEHOLDER',
                          '\n'.join(command_definitions))


def main():
    parser = optparse.OptionParser(
        usage="%prog [...command_definition_files] [options]")
    parser.add_option(
        '--output_cc',
        help=
        "The path to output the CC file to. Note: The header is not generated.")
    parser.add_option(
        '--template_cc',
        help="""The path of the CC file template. TEMPLATE_PLACEHOLDER will be
            replaced by the command definitions""")

    (options, args) = parser.parse_args()

    cc_contents = generate_command_info(args, options.template_cc)

    if options.output_cc:
        with open(options.output_cc, 'w') as f:
            f.write(cc_contents)
    else:
        print(cc_contents)


if __name__ == "__main__":
    main()
