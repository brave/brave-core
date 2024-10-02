// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/ui/commander/bookmark_command_source.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "brave/browser/ui/commander/fuzzy_finder.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/url_and_title.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace commander {

namespace {

std::unique_ptr<CommandItem> CreateOpenBookmarkItem(
    const bookmarks::UrlAndTitle& bookmark,
    Browser* browser) {
  auto item = std::make_unique<CommandItem>();
  item->title = bookmark.title;
  item->entity_type = CommandItem::Entity::kBookmark;
  // base::Unretained is safe because commands are reset when a browser is
  // closed.
  item->command = base::BindOnce(&chrome::AddTabAt, base::Unretained(browser),
                                 GURL(bookmark.url), -1, true, std::nullopt);
  return item;
}

CommandSource::CommandResults GetMatchingBookmarks(
    Browser* browser,
    const std::u16string& input) {
  CommandSource::CommandResults results;
  bookmarks::BookmarkModel* model =
      BookmarkModelFactory::GetForBrowserContext(browser->profile());
  // This should have been checked already.
  DCHECK(model && model->loaded());
  FuzzyFinder finder(input);
  std::vector<gfx::Range> ranges;
  for (bookmarks::UrlAndTitle& bookmark : model->GetUniqueUrls()) {
    double score = finder.Find(bookmark.title, ranges);
    if (score > 0) {
      auto item = CreateOpenBookmarkItem(bookmark, browser);
      item->score = score;
      item->matched_ranges = ranges;
      results.push_back(std::move(item));
    }
  }
  return results;
}

}  // namespace

BookmarkCommandSource::BookmarkCommandSource() = default;
BookmarkCommandSource::~BookmarkCommandSource() = default;

CommandSource::CommandResults BookmarkCommandSource::GetCommands(
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
  double score = finder.Find(open_title, ranges);
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
