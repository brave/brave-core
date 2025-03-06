/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/brave_bookmark_provider.h"

#include "base/containers/contains.h"
#include "brave/components/omnibox/browser/brave_history_quick_provider.h"
#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
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
    match.allowed_to_be_default_match = bump_match;
    if (!bump_match) {
      continue;
    }

    modified = true;
    match.relevance += kContainsQueryBump;

    // Note: By default the BookmarkProvider will fill the URL from the
    // bookmark. This is fine upstream as they don't allow bookmarks to be the
    // default match (it breaks the user editing the input when its default).
    // We set |fill_into_edit| to the input text so what the user typed is not
    // modified.
    match.fill_into_edit = input.text();
  }

  // If we modified any matches, notify listeners so the UI updates.
  if (modified) {
    NotifyListeners(true);
  }
}
