// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "src/chrome/browser/ui/commander/tab_command_source.cc"  // IWYU pragma: export

#include <string>
#include <vector>

#include "base/functional/bind.h"
#include "chrome/browser/ui/commander/command_source.h"
#include "chrome/browser/ui/commander/fuzzy_finder.h"
#include "ui/gfx/range/range.h"

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

  if (CanCloseTabsToLeft(tab_strip_model)) {
    if (auto item = ItemForTitle(u"Close tabs to the left", finder, &ranges)) {
      item->command =
          base::BindOnce(&CloseTabsToLeft, base::Unretained(browser));
      results.push_back(std::move(item));
    }
  }

  if (HasUnpinnedTabs(tab_strip_model)) {
    if (auto item = ItemForTitle(u"Close unpinned tabs", finder, &ranges)) {
      item->command =
          base::BindOnce(&CloseUnpinnedTabs, base::Unretained(browser));
      results.push_back(std::move(item));
    }
  }

  if (CanMoveTabsToExistingWindow(browser)) {
    if (auto item = ItemForTitle(u"Move tabs to window...", finder, &ranges)) {
      item->command = std::make_pair(
          u"Move tabs to window...",
          base::BindRepeating(&MoveTabsToWindowCommandsForWindowsMatching,
                              base::Unretained(browser)));
      results.push_back(std::move(item));
    }
  }

  if (CanAddAllToNewGroup(tab_strip_model)) {
    if (auto item =
            ItemForTitle(u"Move all tabs to new group", finder, &ranges)) {
      item->command =
          base::BindOnce(&AddAllToNewGroup, base::Unretained(browser));
      results.push_back(std::move(item));
    }
  }

  if (auto item =
          ItemForTitle(u"Add tab to existing group...", finder, &ranges)) {
    item->command = std::make_pair(
        u"Add to existing group...",
        base::BindRepeating(&AddTabsToGroupCommandsForGroupsMatching,
                            base::Unretained(browser)));
    results.push_back(std::move(item));
  }

  if (auto item = ItemForTitle(u"Mute all tabs", finder, &ranges)) {
    item->command =
        base::BindOnce(&MuteAllTabs, base::Unretained(browser), false);
    results.push_back(std::move(item));
  }

  if (auto item = ItemForTitle(u"Mute other tabs", finder, &ranges)) {
    item->command =
        base::BindOnce(&MuteAllTabs, base::Unretained(browser), false);
    results.push_back(std::move(item));
  }

  if (HasUnpinnedTabs(tab_strip_model)) {
    if (auto item = ItemForTitle(u"Pin tab...", finder, &ranges)) {
      item->command = std::make_pair(
          u"Pin tab...",
          base::BindRepeating(&TogglePinTabCommandsForTabsMatching,
                              base::Unretained(browser), true));
      results.push_back((std::move(item)));
    }
  }

  if (HasPinnedTabs(tab_strip_model)) {
    if (auto item = ItemForTitle(u"Unpin tab...", finder, &ranges)) {
      item->command = std::make_pair(
          u"Unpin tab...",
          base::BindRepeating(&TogglePinTabCommandsForTabsMatching,
                              base::Unretained(browser), false));
      results.push_back((std::move(item)));
    }

    if (auto item = ItemForTitle(u"Scroll to top", finder, &ranges)) {
      item->command = base::BindOnce(&ScrollToTop, base::Unretained(browser));
      results.push_back(std::move(item));
    }

    if (auto item = ItemForTitle(u"Scroll to bottom", finder, &ranges)) {
      item->command =
          base::BindOnce(&ScrollToBottom, base::Unretained(browser));
      results.push_back(std::move(item));
    }

    if (send_tab_to_self::ShouldDisplayEntryPoint(
            tab_strip_model->GetActiveWebContents())) {
      if (auto item = ItemForTitle(u"Send tab to self...", finder, &ranges)) {
        item->command = base::BindOnce(&chrome::SendTabToSelfFromPageAction,
                                       base::Unretained(browser));
        results.push_back(std::move(item));
      }
    }
  }

  return results;
}

}  // namespace commander
