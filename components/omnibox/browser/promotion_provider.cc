/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/promotion_provider.h"

#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_search_conversion/types.h"
#include "brave/components/brave_search_conversion/utils.h"
#include "build/build_config.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "url/gurl.h"

using brave_search_conversion::ConversionType;
using brave_search_conversion::GetConversionType;
using brave_search_conversion::GetPromoURL;

PromotionProvider::PromotionProvider(PrefService* prefs,
                                     TemplateURLService* template_url_service)
    : AutocompleteProvider(AutocompleteProvider::TYPE_SEARCH),
      prefs_(prefs),
      template_url_service_(template_url_service) {}

void PromotionProvider::Start(const AutocompleteInput& input,
                              bool minimal_changes) {
  matches_.clear();

  if (input.focus_type() != OmniboxFocusType::DEFAULT ||
      input.type() == metrics::OmniboxInputType::EMPTY ||
      input.type() == metrics::OmniboxInputType::URL) {
    return;
  }

  // Check brave search promotion is needed.
  auto type = GetConversionType(prefs_, template_url_service_);
  if (type == ConversionType::kNone)
    return;

  // Add match for search conversion promotion.
  AddMatchForBraveSearchPromotion(input.text());
}

PromotionProvider::~PromotionProvider() = default;

void PromotionProvider::AddMatchForBraveSearchPromotion(
    const std::u16string& input) {
  // Give relatively higher value than other suggestions to prevent hiding this
  // promotion suggest by other suggestions. We want to display this promotion
  // matches as a second or last matches based on conversion type in omnibox
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
  // Contents that displayed at omnibox when this match is selected.
  // Should we use more descriptive such as Try braver search for "input"?
  match.fill_into_edit = contents;
  match.destination_url = promo_url;
  match.contents = contents;
  ACMatchClassifications styles;
  styles.push_back(ACMatchClassification(0, ACMatchClassification::URL));
  match.contents_class = styles;
  matches_.push_back(match);
}
