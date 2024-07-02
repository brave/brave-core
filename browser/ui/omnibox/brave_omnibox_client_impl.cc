/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/omnibox/brave_omnibox_client_impl.h"

#include "base/values.h"
#include "brave/browser/autocomplete/brave_autocomplete_scheme_classifier.h"
#include "brave/browser/search_engines/search_engine_tracker.h"
#include "brave/components/brave_search_conversion/p3a.h"
#include "brave/components/brave_search_conversion/utils.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "brave/components/omnibox/browser/promotion_utils.h"
#include "brave/components/p3a_utils/bucket.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/omnibox/chrome_omnibox_client.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_result.h"
#include "components/omnibox/browser/omnibox_log.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "base/check_is_test.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/misc_metrics/process_misc_metrics.h"
#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"
#endif

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
  p3a_utils::RecordToHistogramBucket("Brave.Omnibox.SearchCount.3",
                                     {0, 5, 10, 20, 50, 100, 500},
                                     number_of_searches);
}

}  // namespace

BraveOmniboxClientImpl::BraveOmniboxClientImpl(LocationBar* location_bar,
                                               Browser* browser,
                                               Profile* profile)
    : ChromeOmniboxClient(location_bar, browser, profile),
      profile_(profile),
      search_engine_tracker_(
          SearchEngineTrackerFactory::GetForBrowserContext(profile)),
      scheme_classifier_(profile),
      search_storage_(profile_->GetPrefs(), kSearchCountPrefName) {
  // Record initial search count p3a value.
  RecordSearchEventP3A(search_storage_.GetWeeklySum());
#if BUILDFLAG(ENABLE_AI_CHAT)
  if (g_brave_browser_process->process_misc_metrics()) {
    ai_chat_metrics_ =
        g_brave_browser_process->process_misc_metrics()->ai_chat_metrics();
    CHECK(ai_chat_metrics_);
  } else {
    CHECK_IS_TEST();
  }
#endif
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

void BraveOmniboxClientImpl::OnURLOpenedFromOmnibox(OmniboxLog* log) {
  if (log->selection.line <= 0) {
    return;
  }
  const auto match = log->result->match_at(log->selection.line);
  if (IsBraveSearchPromotionMatch(match)) {
    brave_search_conversion::p3a::RecordPromoTrigger(
        g_browser_process->local_state(), GetConversionTypeFromMatch(match));
  }
}

void BraveOmniboxClientImpl::OnAutocompleteAccept(
    const GURL& destination_url,
    TemplateURLRef::PostContent* post_content,
    WindowOpenDisposition disposition,
    ui::PageTransition transition,
    AutocompleteMatchType::Type match_type,
    base::TimeTicks match_selection_timestamp,
    bool destination_url_entered_without_scheme,
    bool destination_url_entered_with_http_scheme,
    const std::u16string& text,
    const AutocompleteMatch& match,
    const AutocompleteMatch& alternative_nav_match,
    IDNA2008DeviationCharacter deviation_char_in_hostname) {
  if (IsSearchEvent(match)) {
    // TODO(iefremov): Optimize this.
    search_storage_.AddDelta(1);
    RecordSearchEventP3A(search_storage_.GetWeeklySum());
    if (search_engine_tracker_ != nullptr) {
      search_engine_tracker_->RecordLocationBarQuery();
    }
#if BUILDFLAG(ENABLE_AI_CHAT)
    if (ai_chat_metrics_) {
      ai_chat_metrics_->RecordOmniboxSearchQuery();
    }
#endif
  }
  ChromeOmniboxClient::OnAutocompleteAccept(
      destination_url, post_content, disposition, transition, match_type,
      match_selection_timestamp, destination_url_entered_without_scheme,
      destination_url_entered_with_http_scheme, text, match,
      alternative_nav_match, deviation_char_in_hostname);
}
