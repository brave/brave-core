/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/brave_bookmark_provider.h"

#include "base/containers/contains.h"
#include "base/strings/string_util.h"
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
  constexpr int kExactTitleBump = 550;

  bool modified = false;

  auto lower_text = base::ToLowerASCII(input.text());
  for (auto& match : matches_) {
    if (match.from_keyword) {
      continue;
    }

    // Upstream, bookmarks on the same URL with different refs will be merged
    // into a single match.
    // We don't want this behavior, so that users can jump to any bookmark with
    // a different ref from the Omnibox.
    // For example, if the user has a bookmark for
    // https://www.brave.com/#one, and https://www.brave.com/#two, and they type
    // "brave" in the Omnibox then only the result for
    // https://www.brave.com/#one will be shown (as the ref is not considered
    // when deciding if URLs are duplicates).
    // The solution here is to care about the ref for Bookmark matches when
    // deciding if URLs are duplicates (i.e. by adding it back).
    if (match.destination_url.has_ref()) {
      match.stripped_destination_url = AutocompleteMatch::GURLToStrippedGURL(
          match.destination_url, input, nullptr, std::u16string(), false);
      GURL::Replacements replacements;
      auto ref = match.destination_url.ref();
      replacements.SetRefStr(ref);
      match.stripped_destination_url =
          match.stripped_destination_url.ReplaceComponents(replacements);
    }

    // We only allow the bookmark to be the default match if the input is an
    // exact match for the bookmark title
    auto lower_description = base::ToLowerASCII(match.description);
    if (lower_description != lower_text) {
      continue;
    }

    // By default |contents| is the folder the bookmark is in if there are no
    // matches in the URL. Instead, we want to should show the URL that will be
    // opened so the user knows what will happen when they select the result.
    // Note: Bookmark paths are prefixed with a "/" to indicate they are
    // relative to the bookmark root.
    if (match.contents.starts_with(u"/")) {
      // This is the same formatting used on Bookmark URLs normally.
      match.contents = url_formatter::FormatUrl(
          match.destination_url,
          url_formatter::kFormatUrlOmitHTTPS |
              url_formatter::kFormatUrlOmitDefaults |
              url_formatter::kFormatUrlOmitTrivialSubdomains,
          base::UnescapeRule::SPACES, nullptr, nullptr, nullptr);
      // In this scenario we're matching the title, so its okay to display no
      // matches on the URL.
      match.contents_class = {
          ACMatchClassification(0, ACMatchClassification::URL)};
    }

    match.SetAllowedToBeDefault(input);

    modified = true;
    match.relevance += kExactTitleBump;
  }

  // If we modified any matches, notify listeners so the UI updates.
  if (modified) {
    NotifyListeners(true);
  }
}
