// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/commander/window_command_source.h"

#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

#include "src/chrome/browser/ui/commander/window_command_source.cc"

namespace commander {

BraveWindowCommandSource::BraveWindowCommandSource() = default;
BraveWindowCommandSource::~BraveWindowCommandSource() = default;

CommandSource::CommandResults BraveWindowCommandSource::GetCommands(
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

  double score = finder.Find(open_title, &ranges);
  if (score > 0) {
    auto verb = std::make_unique<CommandItem>(open_title, score, ranges);
    verb->command = std::make_pair(
        open_title, base::BindRepeating(&SwitchCommandsForWindowsMatching,
                                        base::Unretained(browser)));
    results.push_back(std::move(verb));
  }
  score = finder.Find(merge_title, &ranges);
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
