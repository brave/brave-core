/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/brave_search_provider.h"

#include <vector>

#include "base/feature_list.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "brave/components/omnibox/browser/brave_search_suggestion_parser.h"
#include "brave/components/omnibox/buildflags/buildflags.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_match_type.h"
#include "components/omnibox/browser/autocomplete_provider.h"
#include "components/omnibox/browser/autocomplete_provider_client.h"
#include "components/omnibox/browser/omnibox_text_util.h"
#include "components/omnibox/browser/search_provider.h"
#include "components/omnibox/browser/search_suggestion_parser.h"
#include "components/omnibox/common/omnibox_feature_configs.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(ENABLE_STRICT_QUERY_CHECK_FOR_SEARCH_SUGGESTIONS)
#include "brave/components/omnibox/browser/arithmetic_evaluator.h"
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

BraveSearchProvider::BraveSearchProvider(AutocompleteProviderClient* client,
                                         AutocompleteProviderListener* listener)
    : SearchProvider(client, listener) {}

BraveSearchProvider::~BraveSearchProvider() = default;

void BraveSearchProvider::Start(const AutocompleteInput& input,
                                bool minimal_changes) {
  // Everything else keys off `calculator_answer_`, so leaving it empty is all
  // it takes to fall back to the upstream behaviour.
  calculator_answer_.reset();
#if BUILDFLAG(ENABLE_STRICT_QUERY_CHECK_FOR_SEARCH_SUGGESTIONS)
  if (base::FeatureList::IsEnabled(omnibox::kBraveLocalCalculator)) {
    // Evaluated up front because `SearchProvider::Start()` consults
    // `IsQueryPotentiallyPrivate()` while deciding whether to query suggest.
    calculator_answer_ = MaybeEvaluateLocally(input);
  }
#endif

  SearchProvider::Start(input, minimal_changes);
}

void BraveSearchProvider::UpdateMatches() {
  if (calculator_answer_) {
    // Upstream calls this from several places for one input, so replace rather
    // than append -- otherwise the answer would be listed once per call.
    std::erase_if(default_results_.suggest_results, [](const auto& result) {
      return result.type() == AutocompleteMatchType::CALCULATOR;
    });
    // Injected as a suggest result rather than a match so that the usual
    // conversion (BaseSearchProvider::CreateSearchSuggestion) fills in the
    // destination URL, search terms, classifications and dedup keys.
    default_results_.suggest_results.push_back(
        omnibox::MakeCalculatorSuggestResult(
            /*expression=*/input_.text(), *calculator_answer_,
            /*input_text=*/input_.text(), /*entity_info=*/{},
            omnibox_feature_configs::CalcProvider::Get().score,
            /*from_keyword=*/false));
  }

  SearchProvider::UpdateMatches();
}

#if BUILDFLAG(ENABLE_STRICT_QUERY_CHECK_FOR_SEARCH_SUGGESTIONS)
std::optional<std::u16string> BraveSearchProvider::MaybeEvaluateLocally(
    const AutocompleteInput& input) const {
  if (input.IsZeroSuggest() || input.type() == metrics::OmniboxInputType::URL) {
    return std::nullopt;
  }

  // In keyword mode the user picked an engine explicitly, and a keyword
  // suggest request is still sent even for a private query -- its response
  // would rebuild `matches_` and drop anything we added here.
  if (input.in_keyword_mode()) {
    return std::nullopt;
  }

  // Only step in and provide local calculations for queries that would fail the
  // `IsQueryPotentiallyPrivate` check due to having a long number in the query.
  // If the query doesn't meet that criteria, we're safe to pass it along to the
  // search suggestions API.
  if (!search_suggestions::HasLongNumberInQuery(
          base::UTF16ToUTF8(omnibox::SanitizeTextForPaste(input.text())))) {
    return std::nullopt;
  }

  return omnibox::EvaluateArithmeticExpression(input.text());
}
#endif

void BraveSearchProvider::DoHistoryQuery(bool minimal_changes) {
  if (!client()->GetPrefs()->GetBoolean(omnibox::kHistorySuggestionsEnabled)) {
    return;
  }

  SearchProvider::DoHistoryQuery(minimal_changes);
}

bool BraveSearchProvider::IsQueryPotentiallyPrivate() const {
  // Answered locally in `UpdateMatches()`, so the operands never need to leave
  // the browser. Deliberately ahead of everything else: when we can do the
  // arithmetic ourselves there is no reason to ask anyone.
  if (calculator_answer_) {
    return true;
  }

  if (SearchProvider::IsQueryPotentiallyPrivate()) {
    return true;
  }

  if (IsInputPastedFromClipboard()) {
    // We don't want to send username/pwd in clipboard to suggest server
    // accidently.
    VLOG(2) << __func__
            << " : Treat input as private if it's same with clipboard text";
    return true;
  }

#if BUILDFLAG(ENABLE_STRICT_QUERY_CHECK_FOR_SEARCH_SUGGESTIONS)
  if (!IsQuerySafeToSearchSuggestions(
          omnibox::SanitizeTextForPaste(input_.text()))) {
    return true;
  }
#endif

  return false;
}

BraveSearchProvider* BraveSearchProvider::AsBraveSearchProvider() {
  return this;
}

base::AutoReset<bool> BraveSearchProvider::SetInputIsPastedFromClipboard(
    bool is_pasted) {
  return base::AutoReset<bool>(&input_is_pasted_from_clipboard_, is_pasted);
}

bool BraveSearchProvider::IsInputPastedFromClipboard() const {
  return input_is_pasted_from_clipboard_;
}
