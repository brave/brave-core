// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/app/command_utils.h"

#include <iterator>
#include <map>
#include <string>
#include <vector>

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/app/brave_command_ids.h"
#include "brave/components/commands/common/features.h"
#include "chrome/app/chrome_command_ids.h"

#define ADD_UNTRANSLATED_COMMAND(name) \
  {                                    \
    IDC_##name, {                      \
      GetName(#name)                   \
    }                                  \
  }

namespace commands {
namespace {

std::string GetName(const std::string& raw_name) {
  auto words = base::SplitString(raw_name, "_", base::TRIM_WHITESPACE,
                                 base::SPLIT_WANT_NONEMPTY);
  for (auto& word : words) {
    if (word.size() == 1) {
      continue;
    }
    word = word[0] + base::ToLowerASCII(word.substr(1));
  }
  return base::JoinString(words, " ");
}

const base::flat_map<uint16_t, std::string>& GetCommandInfo() {
  static const base::NoDestructor<base::flat_map<uint16_t, std::string>>
      kCommands({
        // Navigation commands.
        ADD_UNTRANSLATED_COMMAND(BACK), ADD_UNTRANSLATED_COMMAND(FORWARD),
            ADD_UNTRANSLATED_COMMAND(RELOAD),
            ADD_UNTRANSLATED_COMMAND(RELOAD_BYPASSING_CACHE),
            ADD_UNTRANSLATED_COMMAND(RELOAD_CLEARING_CACHE),
            ADD_UNTRANSLATED_COMMAND(HOME), ADD_UNTRANSLATED_COMMAND(STOP),

            // Window management commands
            ADD_UNTRANSLATED_COMMAND(NEW_WINDOW),
            ADD_UNTRANSLATED_COMMAND(NEW_INCOGNITO_WINDOW),
            ADD_UNTRANSLATED_COMMAND(CLOSE_WINDOW),
            ADD_UNTRANSLATED_COMMAND(NEW_TAB),
            ADD_UNTRANSLATED_COMMAND(CLOSE_TAB),
            ADD_UNTRANSLATED_COMMAND(SELECT_NEXT_TAB),
            ADD_UNTRANSLATED_COMMAND(SELECT_PREVIOUS_TAB),
            ADD_UNTRANSLATED_COMMAND(SELECT_TAB_0),
            ADD_UNTRANSLATED_COMMAND(SELECT_TAB_1),
            ADD_UNTRANSLATED_COMMAND(SELECT_TAB_2),
            ADD_UNTRANSLATED_COMMAND(SELECT_TAB_3),
            ADD_UNTRANSLATED_COMMAND(SELECT_TAB_4),
            ADD_UNTRANSLATED_COMMAND(SELECT_TAB_5),
            ADD_UNTRANSLATED_COMMAND(SELECT_TAB_6),
            ADD_UNTRANSLATED_COMMAND(SELECT_TAB_7),
            ADD_UNTRANSLATED_COMMAND(SELECT_LAST_TAB),
            ADD_UNTRANSLATED_COMMAND(MOVE_TAB_TO_NEW_WINDOW),
            ADD_UNTRANSLATED_COMMAND(DUPLICATE_TAB),
            ADD_UNTRANSLATED_COMMAND(RESTORE_TAB),
            ADD_UNTRANSLATED_COMMAND(FULLSCREEN),
            ADD_UNTRANSLATED_COMMAND(EXIT),
            ADD_UNTRANSLATED_COMMAND(MOVE_TAB_NEXT),
            ADD_UNTRANSLATED_COMMAND(MOVE_TAB_PREVIOUS),
            ADD_UNTRANSLATED_COMMAND(SEARCH),
            ADD_UNTRANSLATED_COMMAND(DEBUG_FRAME_TOGGLE),
            ADD_UNTRANSLATED_COMMAND(WINDOW_MENU),
            ADD_UNTRANSLATED_COMMAND(MINIMIZE_WINDOW),
            ADD_UNTRANSLATED_COMMAND(MAXIMIZE_WINDOW),
            ADD_UNTRANSLATED_COMMAND(NAME_WINDOW),

#if BUILDFLAG(IS_LINUX)
            ADD_UNTRANSLATED_COMMAND(USE_SYSTEM_TITLE_BAR),
            ADD_UNTRANSLATED_COMMAND(RESTORE_WINDOW),
#endif

            // Web app window commands
            ADD_UNTRANSLATED_COMMAND(OPEN_IN_PWA_WINDOW),
            ADD_UNTRANSLATED_COMMAND(COPY_URL),
            ADD_UNTRANSLATED_COMMAND(SITE_SETTINGS),
            ADD_UNTRANSLATED_COMMAND(WEB_APP_MENU_APP_INFO),

            // Page related commands
            ADD_UNTRANSLATED_COMMAND(BOOKMARK_THIS_TAB),
            ADD_UNTRANSLATED_COMMAND(BOOKMARK_ALL_TABS),
            ADD_UNTRANSLATED_COMMAND(VIEW_SOURCE),
            ADD_UNTRANSLATED_COMMAND(PRINT),
            ADD_UNTRANSLATED_COMMAND(SAVE_PAGE),
            ADD_UNTRANSLATED_COMMAND(EMAIL_PAGE_LOCATION),
            ADD_UNTRANSLATED_COMMAND(BASIC_PRINT),
            ADD_UNTRANSLATED_COMMAND(TRANSLATE_PAGE),
            ADD_UNTRANSLATED_COMMAND(WINDOW_MUTE_SITE),
            ADD_UNTRANSLATED_COMMAND(WINDOW_PIN_TAB),
            ADD_UNTRANSLATED_COMMAND(WINDOW_GROUP_TAB),
            ADD_UNTRANSLATED_COMMAND(QRCODE_GENERATOR),
            ADD_UNTRANSLATED_COMMAND(WINDOW_CLOSE_TABS_TO_RIGHT),
            ADD_UNTRANSLATED_COMMAND(WINDOW_CLOSE_OTHER_TABS),
            ADD_UNTRANSLATED_COMMAND(NEW_TAB_TO_RIGHT),

            // Page manipulation for specific tab
            ADD_UNTRANSLATED_COMMAND(MUTE_TARGET_SITE),
            ADD_UNTRANSLATED_COMMAND(PIN_TARGET_TAB),
            ADD_UNTRANSLATED_COMMAND(GROUP_TARGET_TAB),
            ADD_UNTRANSLATED_COMMAND(DUPLICATE_TARGET_TAB),

            // Edit
            ADD_UNTRANSLATED_COMMAND(CUT), ADD_UNTRANSLATED_COMMAND(COPY),
            ADD_UNTRANSLATED_COMMAND(PASTE),
            ADD_UNTRANSLATED_COMMAND(EDIT_MENU),

            // Find
            ADD_UNTRANSLATED_COMMAND(FIND), ADD_UNTRANSLATED_COMMAND(FIND_NEXT),
            ADD_UNTRANSLATED_COMMAND(FIND_PREVIOUS),
            ADD_UNTRANSLATED_COMMAND(CLOSE_FIND_OR_STOP),

            // Zoom
            ADD_UNTRANSLATED_COMMAND(ZOOM_MENU),
            ADD_UNTRANSLATED_COMMAND(ZOOM_PLUS),
            ADD_UNTRANSLATED_COMMAND(ZOOM_NORMAL),
            ADD_UNTRANSLATED_COMMAND(ZOOM_MINUS),
            ADD_UNTRANSLATED_COMMAND(ZOOM_PERCENT_DISPLAY),

            // Focus
            ADD_UNTRANSLATED_COMMAND(FOCUS_TOOLBAR),
            ADD_UNTRANSLATED_COMMAND(FOCUS_LOCATION),
            ADD_UNTRANSLATED_COMMAND(FOCUS_SEARCH),
            ADD_UNTRANSLATED_COMMAND(FOCUS_MENU_BAR),
            ADD_UNTRANSLATED_COMMAND(FOCUS_NEXT_PANE),
            ADD_UNTRANSLATED_COMMAND(FOCUS_PREVIOUS_PANE),
            ADD_UNTRANSLATED_COMMAND(FOCUS_BOOKMARKS),
            ADD_UNTRANSLATED_COMMAND(FOCUS_INACTIVE_POPUP_FOR_ACCESSIBILITY),
            ADD_UNTRANSLATED_COMMAND(FOCUS_WEB_CONTENTS_PANE),

            // UI bits
            ADD_UNTRANSLATED_COMMAND(OPEN_FILE),
            ADD_UNTRANSLATED_COMMAND(CREATE_SHORTCUT),
            ADD_UNTRANSLATED_COMMAND(DEVELOPER_MENU),
            ADD_UNTRANSLATED_COMMAND(DEV_TOOLS),
            ADD_UNTRANSLATED_COMMAND(DEV_TOOLS_CONSOLE),
            ADD_UNTRANSLATED_COMMAND(TASK_MANAGER),
            ADD_UNTRANSLATED_COMMAND(DEV_TOOLS_DEVICES),
            ADD_UNTRANSLATED_COMMAND(FEEDBACK),
            ADD_UNTRANSLATED_COMMAND(SHOW_BOOKMARK_BAR),
            ADD_UNTRANSLATED_COMMAND(SHOW_HISTORY),
            ADD_UNTRANSLATED_COMMAND(SHOW_BOOKMARK_MANAGER),
            ADD_UNTRANSLATED_COMMAND(SHOW_DOWNLOADS),
            ADD_UNTRANSLATED_COMMAND(CLEAR_BROWSING_DATA),
            ADD_UNTRANSLATED_COMMAND(IMPORT_SETTINGS),
            ADD_UNTRANSLATED_COMMAND(OPTIONS),
            ADD_UNTRANSLATED_COMMAND(EDIT_SEARCH_ENGINES),
            ADD_UNTRANSLATED_COMMAND(VIEW_PASSWORDS),
            ADD_UNTRANSLATED_COMMAND(ABOUT),
            ADD_UNTRANSLATED_COMMAND(HELP_PAGE_VIA_KEYBOARD),
            ADD_UNTRANSLATED_COMMAND(SHOW_APP_MENU),
            ADD_UNTRANSLATED_COMMAND(MANAGE_EXTENSIONS),
            ADD_UNTRANSLATED_COMMAND(DEV_TOOLS_INSPECT),
            ADD_UNTRANSLATED_COMMAND(BOOKMARKS_MENU),
            ADD_UNTRANSLATED_COMMAND(SHOW_AVATAR_MENU),
            ADD_UNTRANSLATED_COMMAND(TOGGLE_REQUEST_TABLET_SITE),
            ADD_UNTRANSLATED_COMMAND(DEV_TOOLS_TOGGLE),
            ADD_UNTRANSLATED_COMMAND(TAKE_SCREENSHOT),
            ADD_UNTRANSLATED_COMMAND(TOGGLE_FULLSCREEN_TOOLBAR),
            ADD_UNTRANSLATED_COMMAND(INSTALL_PWA),
            ADD_UNTRANSLATED_COMMAND(PASTE_AND_GO),
            ADD_UNTRANSLATED_COMMAND(SHOW_FULL_URLS),
            ADD_UNTRANSLATED_COMMAND(CARET_BROWSING_TOGGLE),
            ADD_UNTRANSLATED_COMMAND(TOGGLE_QUICK_COMMANDS),

            // Media
            ADD_UNTRANSLATED_COMMAND(CONTENT_CONTEXT_PLAYPAUSE),
            ADD_UNTRANSLATED_COMMAND(CONTENT_CONTEXT_MUTE),
            ADD_UNTRANSLATED_COMMAND(CONTENT_CONTEXT_LOOP),
            ADD_UNTRANSLATED_COMMAND(CONTENT_CONTEXT_CONTROLS),

#if BUILDFLAG(ENABLE_SCREEN_AI_SERVICE)
            // Screen AI Visual Annotations.
            ADD_UNTRANSLATED_COMMAND(RUN_SCREEN_AI_VISUAL_ANNOTATIONS),
#endif

            // Tab search
            ADD_UNTRANSLATED_COMMAND(TAB_SEARCH),
            ADD_UNTRANSLATED_COMMAND(TAB_SEARCH_CLOSE),

            // Brave Commands
            ADD_UNTRANSLATED_COMMAND(SHOW_BRAVE_REWARDS),
            ADD_UNTRANSLATED_COMMAND(NEW_TOR_CONNECTION_FOR_SITE),
            ADD_UNTRANSLATED_COMMAND(NEW_OFFTHERECORD_WINDOW_TOR),
            ADD_UNTRANSLATED_COMMAND(SHOW_BRAVE_SYNC),
            ADD_UNTRANSLATED_COMMAND(SHOW_BRAVE_WALLET),
            ADD_UNTRANSLATED_COMMAND(ADD_NEW_PROFILE),
            ADD_UNTRANSLATED_COMMAND(OPEN_GUEST_PROFILE),
            ADD_UNTRANSLATED_COMMAND(SHOW_BRAVE_WALLET_PANEL),
            ADD_UNTRANSLATED_COMMAND(SHOW_BRAVE_VPN_PANEL),
            ADD_UNTRANSLATED_COMMAND(TOGGLE_BRAVE_VPN_TOOLBAR_BUTTON),
            ADD_UNTRANSLATED_COMMAND(MANAGE_BRAVE_VPN_PLAN),
            ADD_UNTRANSLATED_COMMAND(TOGGLE_BRAVE_VPN),
            ADD_UNTRANSLATED_COMMAND(COPY_CLEAN_LINK),
            ADD_UNTRANSLATED_COMMAND(SIDEBAR_TOGGLE_POSITION),
            ADD_UNTRANSLATED_COMMAND(TOGGLE_TAB_MUTE)
      });
  return *kCommands;
}

}  // namespace

const std::vector<uint16_t>& GetCommands() {
  DCHECK(base::FeatureList::IsEnabled(features::kBraveCommands))
      << "This should only be used when |kBraveCommands| is enabled.";
  static base::NoDestructor<std::vector<uint16_t>> result([]() {
    std::vector<uint16_t> result;
    base::ranges::transform(
        GetCommandInfo(), std::back_inserter(result),
        /*id projection*/
        &base::flat_map<uint16_t, std::string>::value_type::first);
    return result;
  }());

  return *result;
}

const std::string& GetCommandName(int command_id) {
  DCHECK(base::FeatureList::IsEnabled(features::kBraveCommands))
      << "This should only be used when |kBraveCommands| is enabled.";
  const auto& info = GetCommandInfo();
  auto it = info.find(command_id);
  CHECK(it != info.end()) << "Unknown command " << command_id
                          << ". This function should only be used for known "
                             "commands (i.e. commands in |GetCommandInfo|). "
                             "This command should probably be added.";
  return it->second;
}

}  // namespace commands
