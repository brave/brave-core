/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/omnibox/brave_omnibox_client_impl.h"

#include <algorithm>

#include "base/cxx17_backports.h"
#include "base/metrics/histogram_macros.h"
#include "base/values.h"
#include "brave/browser/autocomplete/brave_autocomplete_scheme_classifier.h"
#include "brave/common/pref_names.h"
#include "brave/components/weekly_storage/weekly_storage.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/omnibox/chrome_omnibox_client.h"
#include "chrome/browser/ui/omnibox/chrome_omnibox_edit_controller.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace {

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
  return false;
}

void RecordSearchEventP3A(uint64_t number_of_searches) {
  constexpr int kIntervals[] = {0, 5, 10, 20, 50, 100, 500};
  const int* it =
      std::lower_bound(kIntervals, std::end(kIntervals), number_of_searches);
  const int answer = it - kIntervals;
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Omnibox.SearchCount", answer,
                             base::size(kIntervals));
}

}  // namespace

BraveOmniboxClientImpl::BraveOmniboxClientImpl(
    OmniboxEditController* controller,
    Profile* profile)
    : ChromeOmniboxClient(controller, profile),
      profile_(profile),
      scheme_classifier_(profile) {
  // Record initial search count p3a value.
  const base::Value* search_p3a =
      profile_->GetPrefs()->GetList(kSearchCountPrefName);
  if (search_p3a->GetList().size() == 0) {
    RecordSearchEventP3A(0);
  }
}

BraveOmniboxClientImpl::~BraveOmniboxClientImpl() {}

void BraveOmniboxClientImpl::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterListPref(kSearchCountPrefName);
}

const AutocompleteSchemeClassifier&
BraveOmniboxClientImpl::GetSchemeClassifier() const {
  return scheme_classifier_;
}

bool BraveOmniboxClientImpl::IsAutocompleteEnabled() const {
  return profile_->GetPrefs()->GetBoolean(kAutocompleteEnabled);
}

void BraveOmniboxClientImpl::OnInputAccepted(const AutocompleteMatch& match) {
  if (IsSearchEvent(match)) {
    // TODO(iefremov): Optimize this.
    WeeklyStorage storage(profile_->GetPrefs(), kSearchCountPrefName);
    storage.AddDelta(1);
    RecordSearchEventP3A(storage.GetWeeklySum());
  }
}
