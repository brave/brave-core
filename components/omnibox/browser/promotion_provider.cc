/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/promotion_provider.h"

#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_search_conversion/types.h"
#include "brave/components/brave_search_conversion/utils.h"
#include "brave/components/omnibox/browser/promotion_utils.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_provider_client.h"
#include "url/gurl.h"

using brave_search_conversion::ConversionType;
using brave_search_conversion::GetConversionType;
using brave_search_conversion::GetPromoURL;

PromotionProvider::PromotionProvider(AutocompleteProviderClient* client)
    : AutocompleteProvider(AutocompleteProvider::TYPE_SEARCH),
      prefs_(client->GetPrefs()),
      template_url_service_(client->GetTemplateURLService()) {}

PromotionProvider::~PromotionProvider() = default;

void PromotionProvider::Start(const AutocompleteInput& input,
                              bool minimal_changes) {
  matches_.clear();

  if (input.focus_type() != metrics::OmniboxFocusType::INTERACTION_DEFAULT ||
      input.type() == metrics::OmniboxInputType::EMPTY ||
      input.type() == metrics::OmniboxInputType::URL) {
    return;
  }

  // Add match for search conversion promotion.
  AddMatchForBraveSearchPromotion(input.text());
}

void PromotionProvider::AddMatchForBraveSearchPromotion(
    const std::u16string& input) {
  // Check brave search promotion is needed.
  auto type = GetConversionType(prefs_, template_url_service_);
  if (type == ConversionType::kNone)
    return;

  // Give relatively higher value than other suggestions to prevent hiding this
  // promotion suggest by other suggestions. We want to display this promotion
  // matches as the second or last match based on conversion type in omnibox
  // popup but it's difficult to handling it here. So, we just use one high
  // relavance value and re-positioning will be done by autocomplete controller.
  constexpr int kRelavance = 800;
  // As this has lower priority than history entries, this search promotion
  // entry could not be visible when other providers supply many related matches
  // from history.
  AutocompleteMatch match(this, kRelavance, false,
                          AutocompleteMatchType::NAVSUGGEST);
  const GURL promo_url = GetPromoURL(input);
  const auto contents = base::UTF8ToUTF16(promo_url.spec());
  // URL is displayed at omnibox edit box when this match is selected.
  match.fill_into_edit = contents;
  match.destination_url = promo_url;
  match.contents = contents;
  ACMatchClassifications styles;
  styles.push_back(ACMatchClassification(0, ACMatchClassification::URL));
  match.contents_class = styles;
  SetConversionTypeToMatch(type, &match);
  matches_.push_back(match);
}
