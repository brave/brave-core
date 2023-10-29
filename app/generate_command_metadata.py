# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
from os import path

# Chromium commands need to be manually added to this list rather than getting
# pulled in automatically because doing it automatically makes rebasing painful.
SUPPORTED_CHROMIUM_COMMANDS = [
    "IDC_ABOUT",
    "IDC_ADD_NEW_PROFILE",
    "IDC_ALL_WINDOWS_FRONT",
    "IDC_BACK",
    "IDC_BASIC_PRINT",
    "IDC_BOOKMARK_ALL_TABS",
    "IDC_BOOKMARK_BAR_ADD_NEW_BOOKMARK",
    "IDC_BOOKMARK_BAR_ADD_TO_BOOKMARKS_BAR",
    "IDC_BOOKMARK_BAR_ALWAYS_SHOW",
    "IDC_BOOKMARK_BAR_EDIT",
    "IDC_BOOKMARK_BAR_NEW_FOLDER",
    "IDC_BOOKMARK_BAR_OPEN_ALL_INCOGNITO",
    "IDC_BOOKMARK_BAR_OPEN_ALL_NEW_TAB_GROUP",
    "IDC_BOOKMARK_BAR_OPEN_ALL_NEW_WINDOW",
    "IDC_BOOKMARK_BAR_OPEN_ALL",
    "IDC_BOOKMARK_BAR_OPEN_INCOGNITO",
    "IDC_BOOKMARK_BAR_REDO",
    "IDC_BOOKMARK_BAR_REMOVE_FROM_BOOKMARKS_BAR",
    "IDC_BOOKMARK_BAR_REMOVE",
    "IDC_BOOKMARK_BAR_RENAME_FOLDER",
    "IDC_BOOKMARK_BAR_SHOW_APPS_SHORTCUT",
    "IDC_BOOKMARK_BAR_SHOW_MANAGED_BOOKMARKS",
    "IDC_BOOKMARK_BAR_UNDO",
    "IDC_BOOKMARK_MANAGER",
    "IDC_BOOKMARK_THIS_TAB",
    "IDC_CARET_BROWSING_TOGGLE",
    "IDC_CHROME_WHATS_NEW",
    "IDC_CLEAR_BROWSING_DATA",
    "IDC_CLOSE_FIND_OR_STOP",
    "IDC_CLOSE_TAB",
    "IDC_CLOSE_WINDOW",
    "IDC_COPY_URL",
    "IDC_COPY",
    "IDC_CREATE_SHORTCUT",
    "IDC_CUSTOMIZE_TOUCH_BAR",
    "IDC_CUT",
    "IDC_DEBUG_PRINT_VIEW_TREE",
    "IDC_DEBUG_TOGGLE_TABLET_MODE",
    "IDC_DEV_TOOLS_CONSOLE",
    "IDC_DEV_TOOLS_DEVICES",
    "IDC_DEV_TOOLS_INSPECT",
    "IDC_DEV_TOOLS_TOGGLE",
    "IDC_DEV_TOOLS",
    "IDC_DISTILL_PAGE",
    "IDC_DUPLICATE_TAB",
    "IDC_EDIT_SEARCH_ENGINES",
    "IDC_EMAIL_PAGE_LOCATION",
    "IDC_EXIT",
    "IDC_FEEDBACK",
    "IDC_FIND_NEXT",
    "IDC_FIND_PREVIOUS",
    "IDC_FIND",
    "IDC_FOCUS_BOOKMARKS",
    "IDC_FOCUS_INACTIVE_POPUP_FOR_ACCESSIBILITY",
    "IDC_FOCUS_LOCATION",
    "IDC_FOCUS_MENU_BAR",
    "IDC_FOCUS_NEXT_PANE",
    "IDC_FOCUS_PREVIOUS_PANE",
    "IDC_FOCUS_SEARCH",
    "IDC_FOCUS_TOOLBAR",
    "IDC_FOCUS_WEB_CONTENTS_PANE",
    "IDC_FORWARD",
    "IDC_FULLSCREEN",
    "IDC_HELP_PAGE_VIA_KEYBOARD",
    "IDC_HOME",
    "IDC_IMPORT_SETTINGS",
    "IDC_INSTALL_PWA",
    "IDC_LIVE_CAPTION",
    "IDC_MANAGE_EXTENSIONS",
    "IDC_MAXIMIZE_WINDOW",
    "IDC_MINIMIZE_WINDOW",
    "IDC_MOVE_TAB_NEXT",
    "IDC_MOVE_TAB_PREVIOUS",
    "IDC_MOVE_TAB_TO_NEW_WINDOW",
    "IDC_NAME_WINDOW",
    "IDC_NEW_INCOGNITO_WINDOW",
    "IDC_NEW_TAB_TO_RIGHT",
    "IDC_NEW_TAB",
    "IDC_NEW_WINDOW",
    "IDC_OFFERS_AND_REWARDS_FOR_PAGE",
    "IDC_OPEN_FILE",
    "IDC_OPEN_GUEST_PROFILE",
    "IDC_OPEN_IN_CHROME",
    "IDC_OPEN_IN_PWA_WINDOW",
    "IDC_OPTIONS",
    "IDC_PASTE_AND_GO",
    "IDC_PASTE",
    "IDC_PERFORMANCE",
    "IDC_PRINT",
    "IDC_QRCODE_GENERATOR",
    "IDC_RECENT_TABS_NO_DEVICE_TABS",
    "IDC_RELOAD_BYPASSING_CACHE",
    "IDC_RELOAD_CLEARING_CACHE",
    "IDC_RELOAD",
    "IDC_RESTORE_TAB",
    "IDC_ROUTE_MEDIA",
    "IDC_SAVE_PAGE",
    "IDC_SEARCH",
    "IDC_SELECT_LAST_TAB",
    "IDC_SELECT_NEXT_TAB",
    "IDC_SELECT_PREVIOUS_TAB",
    "IDC_SELECT_TAB_0",
    "IDC_SELECT_TAB_1",
    "IDC_SELECT_TAB_2",
    "IDC_SELECT_TAB_3",
    "IDC_SELECT_TAB_4",
    "IDC_SELECT_TAB_5",
    "IDC_SELECT_TAB_6",
    "IDC_SELECT_TAB_7",
    "IDC_SHARING_HUB_SCREENSHOT",
    "IDC_SHARING_HUB",
    "IDC_SHOW_ADDRESSES",
    "IDC_SHOW_APP_MENU",
    "IDC_SHOW_AS_TAB",
    "IDC_SHOW_AVATAR_MENU",
    "IDC_SHOW_BOOKMARK_BAR",
    "IDC_SHOW_BOOKMARK_MANAGER",
    "IDC_SHOW_BOOKMARK_SIDE_PANEL",
    "IDC_SHOW_DOWNLOADS",
    "IDC_SHOW_FULL_URLS",
    "IDC_SHOW_HISTORY",
    "IDC_SHOW_MANAGEMENT_PAGE",
    "IDC_SHOW_PASSWORD_MANAGER",
    "IDC_SHOW_PAYMENT_METHODS",
    "IDC_STATUS_TRAY_KEEP_CHROME_RUNNING_IN_BACKGROUND",
    "IDC_STOP",
    "IDC_TAB_SEARCH_CLOSE",
    "IDC_TAB_SEARCH",
    "IDC_TAKE_SCREENSHOT",
    "IDC_TASK_MANAGER",
    "IDC_TOGGLE_FULLSCREEN_TOOLBAR",
    "IDC_TOGGLE_JAVASCRIPT_APPLE_EVENTS",
    "IDC_TOGGLE_QUICK_COMMANDS",
    "IDC_SHOW_TRANSLATE",
    "IDC_UPGRADE_DIALOG",
    "IDC_VIEW_PASSWORDS",
    "IDC_VIEW_SOURCE",
    "IDC_WEB_APP_MENU_APP_INFO",
    "IDC_WEB_APP_SETTINGS",
    "IDC_WINDOW_CLOSE_OTHER_TABS",
    "IDC_WINDOW_CLOSE_TABS_TO_RIGHT",
    "IDC_WINDOW_GROUP_TAB",
    "IDC_WINDOW_MUTE_SITE",
    "IDC_WINDOW_PIN_TAB",
    "IDC_ZOOM_MINUS",
    "IDC_ZOOM_NORMAL",
    "IDC_ZOOM_PLUS",
]

# Prefix indicating the line is a command
COMMAND_PREFIX = "#define IDC_"

# Any lines with one of these prefixes is important - we want to keep command
# #ifdefs to ensure they only show up on platforms they're actually available on
PREFIXES_TO_KEEP = [COMMAND_PREFIX, "IDC_", "#if", "#elif", "#else", "#endif"]

# Commands which shouldn't be included, as they either don't work, crash, or
# aren't acceleratable.
EXCLUDE_COMMANDS = [
    # These content context commands only work from a context menu
    "_CONTEXT_",

    # These commands are in a submenu, which we can't trigger
    "_SUBMENU",

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
    "IDC_SIDEBAR_SHOW_OPTION_MENU",
    "IDC_BRAVE_BOOKMARK_BAR_SUBMENU",
    "IDC_SIDEBAR_SHOW_OPTION_ALWAYS",
    "IDC_SIDEBAR_SHOW_OPTION_MOUSEOVER",
    "IDC_SIDEBAR_SHOW_OPTION_NEVER",
    "IDC_BRAVE_VPN_MENU",

    # Not actually commands
    "IDC_BRAVE_COMMANDS_START",
    "IDC_BRAVE_COMMANDS_LAST",
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
    "IDC_NEW_WINDOW": "IDS_TAB_CXMENU_MOVETOANOTHERNEWWINDOW",
    "IDC_MOVE_TAB_TO_NEW_WINDOW": "IDS_MOVE_TAB_TO_NEW_WINDOW",
    "IDC_FEEDBACK": "IDS_REPORT_AN_ISSUE",
    "IDC_SHOW_DOWNLOADS": "IDS_DOWNLOAD_TITLE",
    "IDC_CLEAR_BROWSING_DATA": "IDS_SETTINGS_CLEAR_BROWSING_DATA",
    "IDC_PRINT": "IDS_PRINT_PREVIEW_PRINT_BUTTON",
    "IDC_PASSWORDS_AND_AUTOFILL_MENU": "IDS_PASSWORDS_AND_AUTOFILL_MENU",
    "IDC_OPEN_FILE": "IDS_OPEN_FILE_DIALOG_TITLE",
    "IDC_CREATE_SHORTCUT": "IDS_APP_HOME_CREATE_SHORTCUT",
    "IDC_WINDOW_CLOSE_OTHER_TABS": "IDS_TAB_CXMENU_CLOSEOTHERTABS",
    "IDC_WINDOW_CLOSE_TABS_TO_RIGHT": "IDS_TAB_CXMENU_CLOSETABSTORIGHT",
    "IDC_NEW_TAB_TO_RIGHT": "IDS_TAB_CXMENU_NEWTABTORIGHT",
    "IDC_ABOUT": "IDS_SETTINGS_ABOUT_PROGRAM",
    "IDC_MANAGE_EXTENSIONS": "IDS_MANAGE_EXTENSIONS",
    "IDC_RECENT_TABS_NO_DEVICE_TABS": "IDS_RECENT_TABS_NO_DEVICE_TABS",
    "IDC_DISTILL_PAGE": "IDS_DISTILL_PAGE",
    "IDC_TOGGLE_QUICK_COMMANDS": "IDS_TOGGLE_QUICK_COMMANDS",
    "IDC_CHROME_WHATS_NEW": "IDS_CHROME_WHATS_NEW",
    "IDC_STATUS_TRAY_KEEP_CHROME_RUNNING_IN_BACKGROUND": \
        "IDS_STATUS_TRAY_KEEP_CHROME_RUNNING_IN_BACKGROUND",
    "IDC_SHOW_BRAVE_REWARDS": "IDS_SHOW_BRAVE_REWARDS",
    "IDC_NEW_TOR_CONNECTION_FOR_SITE": "IDS_NEW_TOR_CONNECTION_FOR_SITE",
    "IDC_NEW_OFFTHERECORD_WINDOW_TOR": "IDS_NEW_OFFTHERECORD_WINDOW_TOR",
    "IDC_SHOW_BRAVE_SYNC": "IDS_SHOW_BRAVE_SYNC",
    "IDC_SHOW_BRAVE_WALLET": "IDS_SHOW_BRAVE_WALLET",
    "IDC_SHOW_BRAVE_WEBCOMPAT_REPORTER": "IDS_SHOW_BRAVE_WEBCOMPAT_REPORTER",
    "IDC_COPY_CLEAN_LINK": "IDS_COPY_CLEAN_LINK",
    "IDC_WINDOW_CLOSE_TABS_TO_LEFT": "IDS_TAB_CXMENU_CLOSETABSTOLEFT"
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
    lines = SUPPORTED_CHROMIUM_COMMANDS
    for command_definition_file in command_definition_files:
        lines += extract_relevant_lines(command_definition_file)

    result = ''
    with open(template_file) as f:
        result = f.read()

    def get_command(line):
        """There are two cases where this line could be a command:
           1. The command is a hardcoded Chromium command (i.e. IDC_BACK)
           2. The command is read from a command ids file: #define IDC_BACK 1
        """
        if line.startswith('IDC_'):
            return line
        if line.startswith(COMMAND_PREFIX):
            return line.split(' ')[1]
        return None

    def get_command_l10n(command):
        if command in EXISTING_TRANSLATIONS:
            return EXISTING_TRANSLATIONS[command]
        return 'IDS_' + command

    def get_line(line):
        command = get_command(line)
        if command:
            return f'  {{{command}, {get_command_l10n(command)}}},'

        return line

    def get_id_line(line):
        command_id = get_command(line)
        return f'  {command_id},' if command_id else line

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
