/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/brave_bookmark_provider.h"

#include "base/containers/contains.h"
#include "brave/components/omnibox/browser/brave_history_quick_provider.h"
#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/bookmark_provider.h"
#include "components/omnibox/browser/omnibox_view.h"
#include "components/prefs/pref_service.h"

BraveBookmarkProvider::~BraveBookmarkProvider() = default;

void BraveBookmarkProvider::Start(const AutocompleteInput& input,
                                  bool minimal_changes) {
  if (!client_->GetPrefs()->GetBoolean(omnibox::kBookmarkSuggestionsEnabled)) {
    matches_.clear();
    return;
  }
  BookmarkProvider::Start(input, minimal_changes);

  if (input.text().empty() || matches_.empty()) {
    return;
  }

  // We need to bump the relevance of the bookmark if we ever want it to rank
  // high enough to be the default match.
  constexpr int kContainsQueryBump = 350;
  bool modified = false;

  auto lower_text = base::ToLowerASCII(input.text());
  for (auto& match : matches_) {
    if (match.from_keyword) {
      continue;
    }

    // We only allow the bookmark to be the default match if the input is
    // literally contained in the title or URL.
    auto lower_contents = base::ToLowerASCII(match.contents);
    auto bump_match =
        base::Contains(lower_contents, lower_text) ||
        base::Contains(base::ToLowerASCII(match.description), lower_text);
    if (!bump_match) {
      continue;
    }

    match.SetAllowedToBeDefault(input);

    modified = true;
    match.relevance += kContainsQueryBump;
  }

  // If we modified any matches, notify listeners so the UI updates.
  if (modified) {
    NotifyListeners(true);
  }
}
