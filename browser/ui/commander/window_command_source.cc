// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/ui/commander/window_command_source.h"

#include <memory>
#include <numeric>
#include <utility>
#include <vector>

#include "brave/browser/ui/commander/entity_match.h"
#include "brave/browser/ui/commander/fuzzy_finder.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace commander {

namespace {

// Activates `browser` if it's still present.
void SwitchToBrowser(base::WeakPtr<Browser> browser) {
  if (browser.get()) {
    browser->window()->Show();
  }
}

// Merges all tabs from `source` into `target`, if they are both present.
void MergeBrowsers(base::WeakPtr<Browser> source,
                   base::WeakPtr<Browser> target) {
  if (!source.get() || !target.get()) {
    return;
  }
  size_t source_count = source->tab_strip_model()->count();
  std::vector<int> indices(source_count);
  std::iota(indices.begin(), indices.end(), 0);
  chrome::MoveTabsToExistingWindow(source.get(), target.get(), indices);
}

// Returns browser windows whose titles fuzzy match `input`. If input is empty,
// returns all eligible browser windows with score reflecting MRU order.
// `browser_to_exclude` is excluded from the list, as are all browser windows
// from a different profile unless `match_profile` is false.

std::unique_ptr<CommandItem> CreateSwitchWindowItem(const WindowMatch& match) {
  auto item = match.ToCommandItem();
  item->command = base::BindOnce(&SwitchToBrowser, match.browser->AsWeakPtr());
  return item;
}

std::unique_ptr<CommandItem> CreateMergeWindowItem(Browser* source,
                                                   const WindowMatch& target) {
  auto item = target.ToCommandItem();
  item->command = base::BindOnce(&MergeBrowsers, source->AsWeakPtr(),
                                 target.browser->AsWeakPtr());
  return item;
}

CommandSource::CommandResults SwitchCommandsForWindowsMatching(
    Browser* browser_to_exclude,
    const std::u16string& input) {
  CommandSource::CommandResults results;
  for (auto& match : WindowsMatchingInput(browser_to_exclude, input)) {
    results.push_back(CreateSwitchWindowItem(match));
  }
  return results;
}

CommandSource::CommandResults MergeCommandsForWindowsMatching(
    Browser* source_browser,
    const std::u16string& input) {
  CommandSource::CommandResults results;
  for (auto& match : WindowsMatchingInput(source_browser, input, true)) {
    results.push_back(CreateMergeWindowItem(source_browser, match));
  }
  return results;
}

}  // namespace

WindowCommandSource::WindowCommandSource() = default;
WindowCommandSource::~WindowCommandSource() = default;

CommandSource::CommandResults WindowCommandSource::GetCommands(
    const std::u16string& input,
    Browser* browser) const {
  CommandSource::CommandResults results;
  BrowserList* browser_list = BrowserList::GetInstance();
  if (browser_list->size() < 2) {
    return results;
  }
  FuzzyFinder finder(input);
  std::vector<gfx::Range> ranges;
  std::u16string open_title =
      l10n_util::GetStringUTF16(IDS_COMMANDER_SWITCH_TO_WINDOW);
  std::u16string merge_title =
      l10n_util::GetStringUTF16(IDS_COMMANDER_MERGE_WINDOW_INTO);

  double score = finder.Find(open_title, ranges);
  if (score > 0) {
    auto verb = std::make_unique<CommandItem>(open_title, score, ranges);
    verb->command = std::make_pair(
        open_title, base::BindRepeating(&SwitchCommandsForWindowsMatching,
                                        base::Unretained(browser)));
    results.push_back(std::move(verb));
  }
  score = finder.Find(merge_title, ranges);
  if (score > 0 && !browser->is_type_devtools()) {
    auto verb = std::make_unique<CommandItem>(merge_title, score, ranges);
    verb->command = std::make_pair(
        merge_title, base::BindRepeating(&MergeCommandsForWindowsMatching,
                                         base::Unretained(browser)));
    results.push_back(std::move(verb));
  }
  return results;
}
}  // namespace commander
