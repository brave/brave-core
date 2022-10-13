/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/omnibox/brave_omnibox_client_impl.h"

#include "base/values.h"
#include "brave/browser/autocomplete/brave_autocomplete_scheme_classifier.h"
#include "brave/components/brave_search_conversion/p3a.h"
#include "brave/components/brave_search_conversion/utils.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "brave/components/omnibox/browser/promotion_utils.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/time_period_storage/weekly_storage.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/omnibox/chrome_omnibox_client.h"
#include "chrome/browser/ui/omnibox/chrome_omnibox_edit_controller.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_result.h"
#include "components/omnibox/browser/omnibox_log.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace {

using brave_search_conversion::ConversionType;
using brave_search_conversion::GetConversionType;

constexpr char kSearchCountPrefName[] = "brave.weekly_storage.search_count";

bool IsSearchEvent(const AutocompleteMatch& match) {
  switch (match.type) {
    case AutocompleteMatchType::SEARCH_WHAT_YOU_TYPED:
    case AutocompleteMatchType::SEARCH_HISTORY:
    case AutocompleteMatchType::SEARCH_SUGGEST:
    case AutocompleteMatchType::SEARCH_SUGGEST_ENTITY:
    case AutocompleteMatchType::SEARCH_SUGGEST_TAIL:
    case AutocompleteMatchType::SEARCH_SUGGEST_PERSONALIZED:
    case AutocompleteMatchType::SEARCH_SUGGEST_PROFILE:
    case AutocompleteMatchType::SEARCH_OTHER_ENGINE:
      return true;
    default:
      return false;
  }
}

void RecordSearchEventP3A(uint64_t number_of_searches) {
  p3a_utils::RecordToHistogramBucket("Brave.Omnibox.SearchCount",
                                     {0, 5, 10, 20, 50, 100, 500},
                                     number_of_searches);
}

}  // namespace

BraveOmniboxClientImpl::BraveOmniboxClientImpl(
    OmniboxEditController* controller,
    Profile* profile)
    : ChromeOmniboxClient(controller, profile),
      profile_(profile),
      scheme_classifier_(profile) {
  // Record initial search count p3a value.
  const auto& search_p3a = profile_->GetPrefs()->GetList(kSearchCountPrefName);
  if (search_p3a.size() == 0) {
    RecordSearchEventP3A(0);
  }
}

BraveOmniboxClientImpl::~BraveOmniboxClientImpl() = default;

void BraveOmniboxClientImpl::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterListPref(kSearchCountPrefName);
}

const AutocompleteSchemeClassifier&
BraveOmniboxClientImpl::GetSchemeClassifier() const {
  return scheme_classifier_;
}

bool BraveOmniboxClientImpl::IsAutocompleteEnabled() const {
  return profile_->GetPrefs()->GetBoolean(omnibox::kAutocompleteEnabled);
}

void BraveOmniboxClientImpl::OnInputAccepted(const AutocompleteMatch& match) {
  if (IsSearchEvent(match)) {
    // TODO(iefremov): Optimize this.
    WeeklyStorage storage(profile_->GetPrefs(), kSearchCountPrefName);
    storage.AddDelta(1);
    RecordSearchEventP3A(storage.GetWeeklySum());
  }
}

void BraveOmniboxClientImpl::OnURLOpenedFromOmnibox(OmniboxLog* log) {
  if (log->selected_index <= 0)
    return;
  const auto match = log->result.match_at(log->selected_index);
  if (IsBraveSearchPromotionMatch(match)) {
    brave_search_conversion::p3a::RecordPromoTrigger(
        g_browser_process->local_state(), GetConversionTypeFromMatch(match));
  }
}
