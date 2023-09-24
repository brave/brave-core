// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/commander/bookmark_command_source.h"

#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

#include "src/chrome/browser/ui/commander/bookmark_command_source.cc"

namespace commander {

BraveBookmarkCommandSource::BraveBookmarkCommandSource() = default;
BraveBookmarkCommandSource::~BraveBookmarkCommandSource() = default;

CommandSource::CommandResults BraveBookmarkCommandSource::GetCommands(
    const std::u16string& input,
    Browser* browser) const {
  CommandSource::CommandResults results;
  bookmarks::BookmarkModel* model =
      BookmarkModelFactory::GetForBrowserContext(browser->profile());
  // Just no-op instead of waiting for the model to load, since this isn't
  // a persistent UI surface and they can just try again.
  if (!model || !model->loaded() || !model->HasBookmarks()) {
    return results;
  }

  FuzzyFinder finder(input);
  std::vector<gfx::Range> ranges;
  std::u16string open_title =
      l10n_util::GetStringUTF16(IDS_COMMANDER_OPEN_BOOKMARK);
  double score = finder.Find(open_title, &ranges);
  if (score > 0) {
    auto verb = std::make_unique<CommandItem>(open_title, score, ranges);
    // base::Unretained is safe because commands are cleared on browser close.
    verb->command = std::make_pair(
        open_title,
        base::BindRepeating(&GetMatchingBookmarks, base::Unretained(browser)));
    results.push_back(std::move(verb));
  }
  return results;
}

}  // namespace commander
