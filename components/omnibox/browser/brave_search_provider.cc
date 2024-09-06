/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/brave_search_provider.h"

#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "brave/components/omnibox/buildflags/buildflags.h"
#include "components/omnibox/browser/autocomplete_provider.h"
#include "components/omnibox/browser/autocomplete_provider_client.h"
#include "components/omnibox/browser/omnibox_view.h"
#include "components/omnibox/browser/search_provider.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(ENABLE_STRICT_QUERY_CHECK_FOR_SEARCH_SUGGESTIONS)
#include "brave/components/omnibox/browser/search_suggestions/query_check_utils.h"
#endif

namespace {

#if BUILDFLAG(ENABLE_STRICT_QUERY_CHECK_FOR_SEARCH_SUGGESTIONS)
bool IsQuerySafeToSearchSuggestions(const std::u16string& query) {
  constexpr int kMaxLength = 50;
  constexpr int kMinSafe = 4;
  // Query too big?
  if (query.length() > kMaxLength) {
    return false;
  }

  // Query small enough?
  if (query.length() <= kMinSafe) {
    return true;
  }

  const base::Time start = base::Time::Now();
  const auto utf8_query = base::UTF16ToUTF8(query);
  if (search_suggestions::IsSuspiciousQuery(utf8_query)) {
    return false;
  }

  if (!search_suggestions::IsSafeQueryUrl(utf8_query)) {
    return false;
  }

  UMA_HISTOGRAM_TIMES("Brave.SearchSuggestions.QueryCheckElapsed",
                      base::Time::Now() - start);

  return true;
}
#endif

}  // namespace

BraveSearchProvider::~BraveSearchProvider() = default;

void BraveSearchProvider::DoHistoryQuery(bool minimal_changes) {
  if (!client()->GetPrefs()->GetBoolean(omnibox::kHistorySuggestionsEnabled))
    return;

  SearchProvider::DoHistoryQuery(minimal_changes);
}

bool BraveSearchProvider::IsQueryPotentiallyPrivate() const {
  if (SearchProvider::IsQueryPotentiallyPrivate()) {
    return true;
  }

  // GetClipboardText() returns after sanitize.
  const auto clipboard_text = client()->GetClipboardText();
  const auto input_text = OmniboxView::SanitizeTextForPaste(input_.text());
  if (input_text.empty()) {
    return false;
  }

  if (input_text == clipboard_text) {
    // We don't want to send username/pwd in clipboard to suggest server
    // accidently.
    VLOG(2) << __func__
            << " : Treat input as private if it's same with clipboard text";
    return true;
  }

#if BUILDFLAG(ENABLE_STRICT_QUERY_CHECK_FOR_SEARCH_SUGGESTIONS)
  if (!IsQuerySafeToSearchSuggestions(input_text)) {
    return true;
  }
#endif

  return false;
}
