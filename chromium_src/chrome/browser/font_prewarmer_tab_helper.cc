// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/font_prewarmer_tab_helper.h"

#include "components/pref_registry/pref_registry_syncable.h"

// Overrides the upstream class to be a no-op. Brave does not prewarm the fonts
// of search results pages, so this observes nothing and never touches prefs.

namespace {

const char kSearchResultsPageFontsPref[] =
    "cached_fonts.search_results_page.fonts";

}  // namespace

// static
void FontPrewarmerTabHelper::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterListPref(kSearchResultsPageFontsPref);
}

FontPrewarmerTabHelper::FontPrewarmerTabHelper(content::WebContents*) {}

FontPrewarmerTabHelper::~FontPrewarmerTabHelper() = default;

// static
std::string FontPrewarmerTabHelper::GetSearchResultsPageFontsPref() {
  return kSearchResultsPageFontsPref;
}

// static
std::vector<std::string> FontPrewarmerTabHelper::GetFontNames(Profile*) {
  return {};
}

void FontPrewarmerTabHelper::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {}

void FontPrewarmerTabHelper::ReadyToCommitNavigation(
    content::NavigationHandle* navigation_handle) {}
