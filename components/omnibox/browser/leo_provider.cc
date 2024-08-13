/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/leo_provider.h"

#include <utility>

#include "base/strings/string_split.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/omnibox/browser/leo_action.h"
#include "components/grit/brave_components_strings.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_provider_client.h"
#include "components/omnibox/browser/keyword_provider.h"
#include "third_party/metrics_proto/omnibox_input_type.pb.h"
#include "third_party/omnibox_proto/types.pb.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

constexpr char kIsMatchFromLeoProviderKey[] = "match-from-brave-leo-provider";

bool IsInputSearchType(const AutocompleteInput& input) {
  switch (input.type()) {
    case metrics::UNKNOWN:
      [[fallthrough]];
    case metrics::QUERY:
      [[fallthrough]];
    case metrics::DEPRECATED_FORCED_QUERY:
      return true;
    default:
      return false;
  }
}

}  // namespace

// static
bool LeoProvider::IsMatchFromLeoProvider(const AutocompleteMatch& match) {
  // TODO: `GetAdditionalInfoForDebugging()` shouldn't be used for non-debugging
  // purposes.
  return !match.GetAdditionalInfoForDebugging(kIsMatchFromLeoProviderKey)
              .empty();
}

LeoProvider::LeoProvider(AutocompleteProviderClient* client)
    : AutocompleteProvider(TYPE_BRAVE_LEO), client_(client) {
  CHECK(base::FeatureList::IsEnabled(ai_chat::features::kAIChat));
  CHECK(client_);
}

void LeoProvider::Start(const AutocompleteInput& input, bool minimal_changes) {
  if (minimal_changes) {
    // when |minimal_changes| is true, input.text() could be same with the
    // previous one.
    return;
  }
  matches_.clear();

  // TODO(sko) We might not want to show Leo suggestion when input was due to
  // focus interaction, like SearchProvider.
  if (!client_->IsLeoProviderEnabled() || !IsInputSearchType(input)) {
    return;
  }

  // This score is approximate number used for keyword search. The
  // |SearchProvider| could add or take away score a little bit, but we don't
  // need that for now.
  constexpr int kRelevance = 1500;

  // Use SEARCH_SUGGEST_ENTITY match type so that the match.description can be
  // visible from OmniboxResultView.
  constexpr AutocompleteMatchType::Type kMatchType =
      AutocompleteMatchType::SEARCH_SUGGEST_ENTITY;

  AutocompleteMatch match(/*provider*/ this, kRelevance, /*deletable*/ false,
                          kMatchType);

  auto text = input.text();
  if (input.InKeywordMode()) {
    KeywordProvider::SplitKeywordFromInput(
        text, /*trim_leading_whitespace=*/true, &text);
  }
  match.keyword = text;
  match.contents = text;
  match.fill_into_edit = text;
  match.contents_class = {
      ACMatchClassification(0, ACMatchClassification::MATCH)};
  match.description =
      l10n_util::GetStringUTF16(IDS_OMNIBOX_ASK_LEO_DESCRIPTION);
  match.description_class = {
      ACMatchClassification(0, ACMatchClassification::DIM)};
  // This must be matched with the |kMatchType|
  match.suggest_type = omnibox::SuggestType::TYPE_ENTITY;
  match.RecordAdditionalInfo(kIsMatchFromLeoProviderKey, true);
  match.takeover_action = base::MakeRefCounted<LeoAction>(text);

  matches_.push_back(std::move(match));

  NotifyListeners(/* updated_matches= */ true);
}

void LeoProvider::Stop(bool clear_cached_results, bool due_to_user_inactivity) {
  matches_.clear();
  AutocompleteProvider::Stop(clear_cached_results, due_to_user_inactivity);
}

LeoProvider::~LeoProvider() = default;
