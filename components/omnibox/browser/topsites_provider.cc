/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/topsites_provider.h"

#include <stddef.h>

#include <algorithm>
#include <string>

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/common/pref_names.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/history_provider.h"
#include "components/prefs/pref_service.h"

// As from autocomplete_provider.h:
// Search Secondary Provider (suggestion)                              |  100++
const int TopSitesProvider::kRelevance = 100;


TopSitesProvider::TopSitesProvider(AutocompleteProviderClient* client)
    : AutocompleteProvider(AutocompleteProvider::TYPE_SEARCH), client_(client) {
}

void TopSitesProvider::Start(const AutocompleteInput& input,
                            bool minimal_changes) {
  matches_.clear();
  auto* prefs = client_->GetPrefs();
  if (!prefs || !prefs->GetBoolean(kTopSiteSuggestionsEnabled)) {
    return;
  }

  if (input.focus_type() != OmniboxFocusType::DEFAULT ||
      (input.type() == metrics::OmniboxInputType::EMPTY) ||
      (input.type() == metrics::OmniboxInputType::QUERY))
    return;

  const std::string input_text =
      base::ToLowerASCII(base::UTF16ToUTF8(input.text()));

  for (std::vector<std::string>::const_iterator i = top_sites_.begin();
       (i != top_sites_.end()) && (matches_.size() < provider_max_matches());
       ++i) {
    const std::string &current_site = *i;
    size_t foundPos = current_site.find(input_text);
    if (std::string::npos != foundPos) {
      ACMatchClassifications styles =
          StylesForSingleMatch(input_text, current_site, foundPos);
      AddMatch(base::ASCIIToUTF16(current_site), styles);
    }
  }

  for (size_t i = 0; i < matches_.size(); ++i) {
    matches_[i].relevance = kRelevance + matches_.size() - (i + 1);
  }
  if ((matches_.size() == 1) && !matches_[0].inline_autocompletion.empty()) {
    // If there's only one possible completion of the user's input and
    // allowing completions truns out to be okay, give the match a high enough
    // score to allow it to beat url-what-you-typed and be inlined.
    matches_[0].SetAllowedToBeDefault(input);
    if (matches_[0].allowed_to_be_default_match) {
      matches_[0].relevance = 1250;
    }
  }
}

TopSitesProvider::~TopSitesProvider() {}

// static
ACMatchClassifications TopSitesProvider::StylesForSingleMatch(
    const std::string &input_text,
    const std::string &site,
    const size_t &foundPos) {
  ACMatchClassifications styles;
  if (foundPos == 0) {
    styles.push_back(ACMatchClassification(
        0, ACMatchClassification::URL | ACMatchClassification::MATCH));
    if (site.length() > input_text.length()) {
      styles.push_back(ACMatchClassification(input_text.length(),
                                             ACMatchClassification::URL));
    }
  } else {
    styles.push_back(ACMatchClassification(0, ACMatchClassification::URL));
    styles.push_back(ACMatchClassification(
        foundPos, ACMatchClassification::URL | ACMatchClassification::MATCH));
    if (site.length() > foundPos + input_text.length()) {
      styles.push_back(
          ACMatchClassification(foundPos + input_text.length(), 0));
    }
  }
  return styles;
}

void TopSitesProvider::AddMatch(const std::u16string& match_string,
                                const ACMatchClassifications& styles) {
  static const std::u16string kScheme(u"https://");
  AutocompleteMatch match(this, kRelevance, false,
                          AutocompleteMatchType::NAVSUGGEST);
  match.fill_into_edit = match_string;
  match.destination_url = GURL(kScheme + match_string);
  match.contents = match_string;
  match.contents_class = styles;
  matches_.push_back(match);
}
