# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
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

    # These commands target a selected tab (i.e. via Ctrl+Click)
    "_TARGET_",

    # These commands are in a submenu, which we can't trigger
    "_SUBMENU",

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
    "IDC_BOOKMARK_BAR_TRACK_PRICE_FOR_SHOPPING_BOOKMARK",
    "IDC_BOOKMARK_BAR_UNTRACK_PRICE_FOR_SHOPPING_BOOKMARK",
    "IDC_CHROME_MENU",
    "IDC_MEDIA_TOOLBAR_CONTEXT_REPORT_CAST_ISSUE",

    # ChromeOS only
    "IDC_TOGGLE_REQUEST_TABLET_SITE",
    "IDC_LACROS_DATA_MIGRATION",
    "IDC_TOGGLE_MULTITASK_MENU",
    "_LRU_USER_",

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
    "IDC_HELP_PAGE_VIA_MENU",  # There's a keyboard command for this
    "IDC_MORE_TOOLS_MENU",
    "IDC_READING_LIST_MENU",
    "IDC_SHARING_HUB_MENU",
    "IDC_MORE_TOOLS_MENU",
    "IDC_BOOKMARKS_LIST_TITLE",
    "IDC_FILE_MENU",
    "IDC_HIDE_APP",
    "IDC_TAB_MENU",
    "IDC_VIEW_MENU",
    "IDC_PROFILE_MAIN_MENU",
    "IDC_INPUT_METHODS_MENU",
    "IDC_MEDIA_ROUTER_ABOUT",
    "IDC_MEDIA_ROUTER_HELP",
    "IDC_MEDIA_ROUTER_LEARN_MORE",
    "IDC_MEDIA_ROUTER_SHOWN_BY_POLICY",
    "IDC_MEDIA_ROUTER_ALWAYS_SHOW_TOOLBAR_ACTION",
    "IDC_MEDIA_ROUTER_TOGGLE_MEDIA_REMOTING",
    "IDC_CLOSE_SIGN_IN_PROMO",
    "IDC_SHOW_SAVE_LOCAL_CARD_SIGN_IN_PROMO_IF_APPLICABLE",
    "IDC_ELEVATED_RECOVERY_DIALOG",
    "IDC_SHOW_SYNC_ERROR",
    "IDC_EXTENSION_ERRORS",
    "IDC_WINDOW_MENU",

    # Not actually commands
    "IDC_BRAVE_COMMANDS_START",
    "IDC_BRAVE_COMMANDS_LAST",
    "IDC_FIRST_UNBOUNDED_MENU",
    "IDC_MANAGE_HID_DEVICES_FIRST",
    "IDC_MANAGE_HID_DEVICES_LAST",
    "IDC_OPEN_LINK_IN_PROFILE_FIRST",
    "IDC_OPEN_LINK_IN_PROFILE_LAST",
    "IDC_SHOW_SETTINGS_CHANGE_FIRST",
    "IDC_SHOW_SETTINGS_CHANGE_LAST",
    "IDC_EXTENSION_INSTALL_ERROR_FIRST",
    "IDC_EXTENSION_INSTALL_ERROR_LAST",
    "IDC_DEVICE_SYSTEM_TRAY_ICON_FIRST",
    "IDC_DEVICE_SYSTEM_TRAY_ICON_LAST"
]

# A number of commands have existing good translations which we can reuse.
# Unfortunately, we can't just do a direct lookup of the command names, as some
# of these expect some existing context to be present, which it won't be for
# commands and shortcuts.
# For example IDC_SIDEBAR_SHOW_OPTION_ALWAYS is the string 'Always' because it
# expects to be show in the context of
# Show Sidebar:
#   - Always
#   - Never
#   - On Hover
# but a command for 'Always' doesn't make a lot of sense.
EXISTING_TRANSLATIONS = {
    "IDC_RELOAD": "IDS_RELOAD",
    "IDC_NEW_WINDOW": "IDS_NEW_WINDOW",
    "IDC_NEW_INCOGNITO_WINDOW": "IDS_NEW_INCOGNITO_WINDOW",
    "IDC_PIN_TO_START_SCREEN": "IDS_PIN_TO_START_SCREEN",
    "IDC_NEW_TAB": "IDS_NEW_TAB",
    "IDC_RESTORE_TAB": "IDS_RESTORE_TAB",
    "IDC_SHOW_AS_TAB": "IDS_SHOW_AS_TAB",
    "IDC_FULLSCREEN": "IDS_FULLSCREEN",
    "IDC_EXIT": "IDS_EXIT",
    "IDC_NAME_WINDOW": "IDS_NAME_WINDOW",
    "IDC_RESTORE_WINDOW": "IDS_RESTORE_WINDOW",
    "IDC_MOVE_TAB_TO_NEW_WINDOW": "IDS_MOVE_TAB_TO_NEW_WINDOW",
    "IDC_COPY_URL": "IDS_COPY_URL",
    "IDC_OPEN_IN_CHROME": "IDS_OPEN_IN_CHROME",
    "IDC_VIEW_SOURCE": "IDS_VIEW_SOURCE",
    "IDC_PRINT": "IDS_PRINT",
    "IDC_SAVE_PAGE": "IDS_SAVE_PAGE",
    "IDC_AUTOFILL_MENU": "IDS_AUTOFILL_MENU",
    "IDC_CUT": "IDS_CUT",
    "IDC_COPY": "IDS_COPY",
    "IDC_PASTE": "IDS_PASTE",
    "IDC_FIND": "IDS_FIND",
    "IDC_FIND_AND_EDIT_MENU": "IDS_FIND_AND_EDIT_MENU",
    "IDC_DEV_TOOLS": "IDS_DEV_TOOLS",
    "IDC_OPEN_FILE": "IDS_OPEN_FILE_DIALOG_TITLE",
    "IDC_CREATE_SHORTCUT": "IDS_APP_HOME_CREATE_SHORTCUT",
    "IDC_WINDOW_CLOSE_OTHER_TABS": "IDS_TAB_CXMENU_CLOSEOTHERTABS",
    "IDC_WINDOW_CLOSE_TABS_TO_RIGHT": "IDS_TAB_CXMENU_CLOSETABSTORIGHT",
    "IDC_NEW_TAB_TO_RIGHT": "IDS_TAB_CXMENU_NEWTABTORIGHT",
    "IDC_DEV_TOOLS_CONSOLE": "IDS_DEV_TOOLS_CONSOLE",
    "IDC_TASK_MANAGER": "IDS_TASK_MANAGER",
    "IDC_DEV_TOOLS_DEVICES": "IDS_DEV_TOOLS_DEVICES",
    "IDC_FEEDBACK": "IDS_FEEDBACK",
    "IDC_SHOW_DOWNLOADS": "IDS_SHOW_DOWNLOADS",
    "IDC_CLEAR_BROWSING_DATA": "IDS_CLEAR_BROWSING_DATA",
    "IDC_OPTIONS": "IDS_OPTIONS",
    "IDC_VIEW_PASSWORDS": "IDS_VIEW_PASSWORDS",
    "IDC_ABOUT": "IDS_ABOUT",
    "IDC_MANAGE_EXTENSIONS": "IDS_MANAGE_EXTENSIONS",
    "IDC_PROFILING_ENABLED": "IDS_PROFILING_ENABLED",
    "IDC_RECENT_TABS_NO_DEVICE_TABS": "IDS_RECENT_TABS_NO_DEVICE_TABS",
    "IDC_DISTILL_PAGE": "IDS_DISTILL_PAGE",
    "IDC_TAKE_SCREENSHOT": "IDS_TAKE_SCREENSHOT",
    "IDC_TOGGLE_QUICK_COMMANDS": "IDS_TOGGLE_QUICK_COMMANDS",
    "IDC_CHROME_TIPS": "IDS_CHROME_TIPS",
    "IDC_CHROME_WHATS_NEW": "IDS_CHROME_WHATS_NEW",
    "IDC_HISTORY_MENU": "IDS_HISTORY_MENU",
    "IDC_STATUS_TRAY_KEEP_CHROME_RUNNING_IN_BACKGROUND": \
        "IDS_STATUS_TRAY_KEEP_CHROME_RUNNING_IN_BACKGROUND",
    "IDC_SHOW_BRAVE_REWARDS": "IDS_SHOW_BRAVE_REWARDS",
    "IDC_NEW_TOR_CONNECTION_FOR_SITE": "IDS_NEW_TOR_CONNECTION_FOR_SITE",
    "IDC_NEW_OFFTHERECORD_WINDOW_TOR": "IDS_NEW_OFFTHERECORD_WINDOW_TOR",
    "IDC_SHOW_BRAVE_SYNC": "IDS_SHOW_BRAVE_SYNC",
    "IDC_SHOW_BRAVE_WALLET": "IDS_SHOW_BRAVE_WALLET",
    "IDC_ADD_NEW_PROFILE": "IDS_ADD_NEW_PROFILE",
    "IDC_OPEN_GUEST_PROFILE": "IDS_OPEN_GUEST_PROFILE",
    "IDC_SHOW_BRAVE_WEBCOMPAT_REPORTER": "IDS_SHOW_BRAVE_WEBCOMPAT_REPORTER",
    "IDC_COPY_CLEAN_LINK": "IDS_COPY_CLEAN_LINK",
}


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

    def get_command(line):
        return line.split(' ')[1]

    def get_command_l10n(line):
        command = get_command(line)
        if command in EXISTING_TRANSLATIONS:
            return EXISTING_TRANSLATIONS[command]
        return 'IDS_' + command

    def get_line(line):
        if line.startswith(COMMAND_PREFIX):
            return f'  {{{get_id(line)}, {get_command_l10n(line)}}},'

        return line

    def get_id(line):
        if line.startswith(COMMAND_PREFIX):
            return get_command(line)
        return line

    def get_id_line(line):
        command_id = get_id(line)
        return command_id if command_id.startswith('#') else f'  {command_id},'

    command_definitions = map(get_line, lines)
    command_ids = map(get_id_line, lines)
    return result.replace('COMMAND_NAMES',
                          '\n'.join(command_definitions)) \
                 .replace('COMMAND_IDS', '\n'.join(command_ids))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("definitions", nargs='+')
    parser.add_argument(
        '--output_cc',
        help=
        "The path to output the CC file to. Note: The header is not generated.")
    parser.add_argument(
        '--template_cc',
        help="""The path of the CC file template. TEMPLATE_PLACEHOLDER will be
            replaced by the command definitions""")

    args = parser.parse_args()

    cc_contents = generate_command_info(args.definitions, args.template_cc)

    if args.output_cc:
        with open(args.output_cc, 'w') as f:
            f.write(cc_contents)
    else:
        print(cc_contents)


if __name__ == "__main__":
    main()
