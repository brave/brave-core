// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/ui/commander/tab_command_source.h"

#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/ranges/algorithm.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/app/command_utils.h"
#include "brave/browser/ui/commander/entity_match.h"
#include "brave/browser/ui/commander/fuzzy_finder.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/send_tab_to_self/send_tab_to_self_util.h"
#include "chrome/browser/ui/accelerator_utils.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/grit/generated_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/sessions/content/session_tab_helper.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/list_selection_model.h"

namespace commander {

namespace {

// TODO(lgrey): It *might* make to pull this out later into a CommandSource
// method or a free function in some common place. Not committing yet.
std::unique_ptr<CommandItem> ItemForTitle(const std::u16string& title,
                                          FuzzyFinder& finder,
                                          std::vector<gfx::Range>& ranges) {
  double score = finder.Find(title, ranges);
  if (score > 0) {
    return std::make_unique<CommandItem>(title, score, ranges);
  }
  return nullptr;
}

// Returns the tab group that the currently selected tabs can *not* be moved to.
// In practice, this is the tab group that *all* selected tabs belong to, if
// any. In the common special case of single selection, this will return that
// tab's group if it has one.
std::optional<tab_groups::TabGroupId> IneligibleGroupForSelected(
    TabStripModel* tab_strip_model) {
  std::optional<tab_groups::TabGroupId> excluded_group = std::nullopt;
  for (int index : tab_strip_model->selection_model().selected_indices()) {
    auto group = tab_strip_model->GetTabGroupForTab(index);
    if (group.has_value()) {
      if (!excluded_group.has_value()) {
        excluded_group = group;
      } else if (group != excluded_group) {
        // More than one group in the selection, so don't exclude anything.
        return std::nullopt;
      }
    }
  }
  return excluded_group;
}

// Returns true only if `browser` is alive, and the contents at `index` match
// `tab_session_id`.
bool DoesTabAtIndexMatchSessionId(base::WeakPtr<Browser> browser,
                                  int index,
                                  int tab_session_id) {
  if (!browser.get()) {
    return false;
  }
  if (browser->tab_strip_model()->count() <= index) {
    return false;
  }
  content::WebContents* contents =
      browser->tab_strip_model()->GetWebContentsAt(index);
  DCHECK(contents);
  return sessions::SessionTabHelper::IdForTab(contents).id() == tab_session_id;
}

// Commands:

bool HasUnpinnedTabs(const TabStripModel* model) {
  return model->IndexOfFirstNonPinnedTab() < model->count();
}

bool HasPinnedTabs(const TabStripModel* model) {
  return model->IndexOfFirstNonPinnedTab() > 0;
}

bool CanMoveTabsToExistingWindow(const Browser* browser_to_exclude) {
  const BrowserList* browser_list = BrowserList::GetInstance();
  return base::ranges::any_of(
      *browser_list, [browser_to_exclude](Browser* browser) {
        return browser != browser_to_exclude && browser->is_type_normal() &&
               browser->profile() == browser_to_exclude->profile();
      });
}

void MoveTabsToExistingWindow(base::WeakPtr<Browser> source,
                              base::WeakPtr<Browser> target) {
  if (!source.get() || !target.get()) {
    return;
  }
  const ui::ListSelectionModel::SelectedIndices& sel =
      source->tab_strip_model()->selection_model().selected_indices();
  chrome::MoveTabsToExistingWindow(source.get(), target.get(),
                                   std::vector<int>(sel.begin(), sel.end()));
}

void AddSelectedToNewGroup(Browser* browser) {
  TabStripModel* model = browser->tab_strip_model();
  const ui::ListSelectionModel::SelectedIndices& sel =
      model->selection_model().selected_indices();
  model->AddToNewGroup(std::vector<int>(sel.begin(), sel.end()));
}

// Multiphase commands:

void TogglePinTab(base::WeakPtr<Browser> browser,
                  int tab_index,
                  int tab_session_id,
                  bool pin) {
  if (!DoesTabAtIndexMatchSessionId(browser, tab_index, tab_session_id)) {
    return;
  }
  browser->tab_strip_model()->SetTabPinned(tab_index, pin);
}

std::unique_ptr<CommandItem> CreatePinTabItem(const TabMatch& match,
                                              Browser* browser,
                                              bool pin) {
  auto item = match.ToCommandItem();
  item->command = base::BindOnce(&TogglePinTab, browser->AsWeakPtr(),
                                 match.index, match.session_id, pin);
  return item;
}

CommandSource::CommandResults TogglePinTabCommandsForTabsMatching(
    Browser* browser,
    bool pin,
    const std::u16string& input) {
  CommandSource::CommandResults results;
  TabSearchOptions options;
  if (pin) {
    options.only_unpinned = true;
  } else {
    options.only_pinned = true;
  }
  for (auto& match : TabsMatchingInput(browser, input, options)) {
    results.push_back(CreatePinTabItem(match, browser, pin));
  }
  return results;
}

std::unique_ptr<CommandItem> CreateMoveTabsToWindowItem(
    Browser* source,
    const WindowMatch& match) {
  auto item = match.ToCommandItem();
  item->command = base::BindOnce(&MoveTabsToExistingWindow, source->AsWeakPtr(),
                                 match.browser->AsWeakPtr());
  return item;
}

CommandSource::CommandResults MoveTabsToWindowCommandsForWindowsMatching(
    Browser* source,
    const std::u16string& input) {
  CommandSource::CommandResults results;
  // Add "New Window", if appropriate. It should score highest with no input.
  std::u16string new_window_title =
      base::UTF8ToUTF16(commands::GetCommandName(IDC_NEW_WINDOW));
  std::unique_ptr<CommandItem> item;
  if (input.empty()) {
    item = std::make_unique<CommandItem>(new_window_title, .99,
                                         std::vector<gfx::Range>());
  } else {
    FuzzyFinder finder(input);
    std::vector<gfx::Range> ranges;
    item = ItemForTitle(new_window_title, finder, ranges);
  }
  if (item) {
    item->entity_type = CommandItem::Entity::kWindow;
    item->command = base::BindOnce(&chrome::MoveActiveTabToNewWindow,
                                   base::Unretained(source));
    results.push_back(std::move(item));
  }
  for (auto& match : WindowsMatchingInput(source, input)) {
    results.push_back(CreateMoveTabsToWindowItem(source, match));
  }
  return results;
}

void AddTabsToGroup(base::WeakPtr<Browser> browser,
                    tab_groups::TabGroupId group) {
  if (!browser.get()) {
    return;
  }
  const ui::ListSelectionModel::SelectedIndices& sel =
      browser->tab_strip_model()->selection_model().selected_indices();
  browser->tab_strip_model()->AddToExistingGroup(
      std::vector<int>(sel.begin(), sel.end()), group);
}

CommandSource::CommandResults AddTabsToGroupCommandsForGroupsMatching(
    Browser* browser,
    const std::u16string& input) {
  CommandSource::CommandResults results;
  TabStripModel* tab_strip_model = browser->tab_strip_model();
  // Add "New Group", if appropriate. It should score highest with no input.
  std::u16string new_group_title =
      l10n_util::GetStringUTF16(IDS_TAB_CXMENU_SUBMENU_NEW_GROUP);
  std::unique_ptr<CommandItem> item;
  if (input.empty()) {
    item = std::make_unique<CommandItem>(new_group_title, .99,
                                         std::vector<gfx::Range>());
  } else {
    FuzzyFinder finder(input);
    std::vector<gfx::Range> ranges;
    item = ItemForTitle(new_group_title, finder, ranges);
  }
  if (item) {
    item->entity_type = CommandItem::Entity::kGroup;
    item->command =
        base::BindOnce(&AddSelectedToNewGroup, base::Unretained(browser));
    results.push_back(std::move(item));
  }
  for (auto& match : GroupsMatchingInput(
           browser, input, IneligibleGroupForSelected(tab_strip_model))) {
    auto command_item = match.ToCommandItem();
    command_item->command =
        base::BindOnce(&AddTabsToGroup, browser->AsWeakPtr(), match.group);
    results.push_back(std::move(command_item));
  }
  return results;
}

}  // namespace

TabCommandSource::TabCommandSource() = default;
TabCommandSource::~TabCommandSource() = default;

CommandSource::CommandResults TabCommandSource::GetCommands(
    const std::u16string& input,
    Browser* browser) const {
  CommandSource::CommandResults results;
  FuzzyFinder finder(input);
  std::vector<gfx::Range> ranges;

  TabStripModel* tab_strip_model = browser->tab_strip_model();

  if (CanMoveTabsToExistingWindow(browser)) {
    auto text = l10n_util::GetStringUTF16(IDS_COMMANDER_MOVE_TABS_TO_WINDOW);
    if (auto item = ItemForTitle(text, finder, ranges)) {
      item->command = std::make_pair(
          text, base::BindRepeating(&MoveTabsToWindowCommandsForWindowsMatching,
                                    base::Unretained(browser)));
      results.push_back(std::move(item));
    }
  }

  auto add_tab_to_existing_group =
      l10n_util::GetStringUTF16(IDS_COMMANDER_ADD_TABS_TO_EXISTING_GROUP);
  if (auto item = ItemForTitle(add_tab_to_existing_group, finder, ranges)) {
    item->command = std::make_pair(
        add_tab_to_existing_group,
        base::BindRepeating(&AddTabsToGroupCommandsForGroupsMatching,
                            base::Unretained(browser)));
    results.push_back(std::move(item));
  }

  if (HasUnpinnedTabs(tab_strip_model)) {
    auto text = l10n_util::GetStringUTF16(IDS_COMMANDER_PIN_TAB);
    if (auto item = ItemForTitle(text, finder, ranges)) {
      item->command = std::make_pair(
          text, base::BindRepeating(&TogglePinTabCommandsForTabsMatching,
                                    base::Unretained(browser), true));
      results.push_back((std::move(item)));
    }
  }

  if (HasPinnedTabs(tab_strip_model)) {
    auto text = l10n_util::GetStringUTF16(IDS_COMMANDER_UNPIN_TAB);
    if (auto item = ItemForTitle(text, finder, ranges)) {
      item->command = std::make_pair(
          text, base::BindRepeating(&TogglePinTabCommandsForTabsMatching,
                                    base::Unretained(browser), false));
      results.push_back((std::move(item)));
    }
  }

  return results;
}

}  // namespace commander
