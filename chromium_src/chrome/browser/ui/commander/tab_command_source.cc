// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/commander/tab_command_source.h"

#include <string>
#include <vector>

#include "base/functional/bind.h"
#include "chrome/browser/ui/commander/command_source.h"
#include "chrome/browser/ui/commander/fuzzy_finder.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/range/range.h"

#include "src/chrome/browser/ui/commander/tab_command_source.cc"

namespace commander {

BraveTabCommandSource::BraveTabCommandSource() = default;
BraveTabCommandSource::~BraveTabCommandSource() = default;

CommandSource::CommandResults BraveTabCommandSource::GetCommands(
    const std::u16string& input,
    Browser* browser) const {
  CommandSource::CommandResults results;
  FuzzyFinder finder(input);
  std::vector<gfx::Range> ranges;

  TabStripModel* tab_strip_model = browser->tab_strip_model();

  if (CanMoveTabsToExistingWindow(browser)) {
    auto text = l10n_util::GetStringUTF16(IDS_COMMANDER_MOVE_TABS_TO_WINDOW);
    if (auto item = ItemForTitle(text, finder, &ranges)) {
      item->command = std::make_pair(
          text, base::BindRepeating(&MoveTabsToWindowCommandsForWindowsMatching,
                                    base::Unretained(browser)));
      results.push_back(std::move(item));
    }
  }

  auto add_tab_to_existing_group =
      l10n_util::GetStringUTF16(IDS_COMMANDER_ADD_TABS_TO_EXISTING_GROUP);
  if (auto item = ItemForTitle(add_tab_to_existing_group, finder, &ranges)) {
    item->command = std::make_pair(
        add_tab_to_existing_group,
        base::BindRepeating(&AddTabsToGroupCommandsForGroupsMatching,
                            base::Unretained(browser)));
    results.push_back(std::move(item));
  }

  if (HasUnpinnedTabs(tab_strip_model)) {
    auto text = l10n_util::GetStringUTF16(IDS_COMMANDER_PIN_TAB);
    if (auto item = ItemForTitle(text, finder, &ranges)) {
      item->command = std::make_pair(
          text, base::BindRepeating(&TogglePinTabCommandsForTabsMatching,
                                    base::Unretained(browser), true));
      results.push_back((std::move(item)));
    }
  }

  if (HasPinnedTabs(tab_strip_model)) {
    auto text = l10n_util::GetStringUTF16(IDS_COMMANDER_UNPIN_TAB);
    if (auto item = ItemForTitle(text, finder, &ranges)) {
      item->command = std::make_pair(
          text, base::BindRepeating(&TogglePinTabCommandsForTabsMatching,
                                    base::Unretained(browser), false));
      results.push_back((std::move(item)));
    }
  }

  return results;
}

}  // namespace commander
