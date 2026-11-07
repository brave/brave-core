// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/components/omnibox/browser/brave_history_embeddings_provider.h"

#include <algorithm>
#include <string>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/history_embeddings/core/history_embeddings_features.h"
#include "brave/components/history_embeddings/core/history_embeddings_search.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_match_classification.h"
#include "components/omnibox/browser/autocomplete_provider.h"
#include "components/omnibox/browser/autocomplete_provider_client.h"
#include "components/omnibox/browser/keyword_provider.h"

namespace {
// The max relevance a history embedding match will receive.
constexpr int kMaxRelevance = 1000;
}  // namespace

BraveHistoryEmbeddingsProvider::BraveHistoryEmbeddingsProvider(
    AutocompleteProviderClient* client,
    AutocompleteProviderListener* listener)
    : HistoryProvider(AutocompleteProvider::Type::kHistoryEmbeddings, client) {
  AddListener(listener);
}

BraveHistoryEmbeddingsProvider::~BraveHistoryEmbeddingsProvider() = default;

void BraveHistoryEmbeddingsProvider::Start(const AutocompleteInput& input,
                                           bool minimal_changes) {
  done_ = true;
  matches_.clear();

  if (input.omit_asynchronous_matches()) {
    return;
  }

  brave_history_embeddings::HistoryEmbeddingsSearch* search =
      client()->GetBraveHistoryEmbeddingsSearch();
  if (!search) {
    return;
  }

  // Remove the keyword from input if we're in keyword mode for a starter pack
  // engine.
  const auto [adjusted_input, starter_pack_engine] =
      AdjustInputForStarterPackKeyword(input,
                                       client()->GetTemplateURLService());
  input_ = adjusted_input;
  starter_pack_engine_ = starter_pack_engine;

  int num_terms =
      brave_history_embeddings::CountWords(base::UTF16ToUTF8(input_.text()));
  if (num_terms < brave_history_embeddings::GetFeatureParameters()
                      .search_query_minimum_word_count) {
    return;
  }

  done_ = false;
  search->Search(nullptr, base::UTF16ToUTF8(input_.text()), {},
                 provider_max_matches_,
                 /*url_id_filter=*/{},
                 base::BindRepeating(
                     &BraveHistoryEmbeddingsProvider::OnReceivedSearchResult,
                     weak_factory_.GetWeakPtr()));
}

void BraveHistoryEmbeddingsProvider::OnReceivedSearchResult(
    brave_history_embeddings::SearchResult search_result) {
  // Check `done_` in case the stop timer fired or the user closed the omnibox
  // before `Search()` completed. Check `last_search_input_` in case this is the
  // result for an earlier `Search()` request; there's usually 2 requests
  // ongoing as the user types.
  if (done_ || search_result.query != base::UTF16ToUTF8(input_.text())) {
    return;
  }

  // `OnReceivedSearchResult()` can be called multiple times per `Search()`
  // request. Clear `matches_``to avoid aggregating duplicates.
  matches_.clear();

  for (const brave_history_embeddings::ScoredUrlRow& scored_url_row :
       search_result.scored_url_rows) {
    matches_.push_back(CreateMatch(scored_url_row));
  }

  done_ = true;
  NotifyListeners(!matches_.empty());
}

AutocompleteMatch BraveHistoryEmbeddingsProvider::CreateMatch(
    const brave_history_embeddings::ScoredUrlRow& scored_url_row) {
  AutocompleteMatch match(
      this, std::min(scored_url_row.scored_url.score, 1.f) * kMaxRelevance,
      client()->AllowDeletingBrowserHistory(),
      omnibox::AutocompleteMatchType::kHistoryEmbeddings);
  match.destination_url = scored_url_row.row.url();

  match.description =
      AutocompleteMatch::SanitizeString(scored_url_row.row.title());
  match.description_class = ClassifyTermMatches(
      FindTermMatches(input_.text(), match.description),
      match.description.size(), ACMatchClassification::MATCH,
      ACMatchClassification::NONE);

  match.contents = base::UTF8ToUTF16(scored_url_row.row.url().spec());
  match.contents_class = ClassifyTermMatches(
      FindTermMatches(input_.text(), match.contents), match.contents.size(),
      ACMatchClassification::MATCH | ACMatchClassification::URL,
      ACMatchClassification::URL);

  if (starter_pack_engine_) {
    match.keyword = starter_pack_engine_->keyword();
    match.transition = ui::PAGE_TRANSITION_KEYWORD;
  }

  match.RecordAdditionalInfo("passages", scored_url_row.GetBestPassage());

  return match;
}
