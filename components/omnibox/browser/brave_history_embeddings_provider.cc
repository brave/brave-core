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
#include <optional>
#include <string>
#include <vector>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/i18n/time_formatting.h"
#include "base/strings/utf_string_conversions.h"
#include "components/history_clusters/core/history_clusters_util.h"
#include "brave/components/history_embeddings/core/history_embeddings_features.h"
#include "brave/components/history_embeddings/core/history_embeddings_search.h"
#include "components/omnibox/browser/autocomplete_enums.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_match_classification.h"
#include "components/omnibox/browser/autocomplete_provider.h"
#include "components/omnibox/browser/autocomplete_provider_client.h"
#include "components/omnibox/browser/keyword_provider.h"
#include "components/omnibox/browser/omnibox_triggered_feature_service.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace {
// The max relevance a history embedding match will receive.
constexpr int kMaxRelevance = 1000;

// Whether the received `status` is expected to be the last or there should be
// more updates..
bool IsAnswerDone(brave_history_embeddings::ComputeAnswerStatus status) {
  switch (status) {
    case brave_history_embeddings::ComputeAnswerStatus::kUnspecified:
    case brave_history_embeddings::ComputeAnswerStatus::kLoading:
      return false;
    case brave_history_embeddings::ComputeAnswerStatus::kSuccess:
    case brave_history_embeddings::ComputeAnswerStatus::kUnanswerable:
    case brave_history_embeddings::ComputeAnswerStatus::kModelUnavailable:
    case brave_history_embeddings::ComputeAnswerStatus::kExecutionFailure:
    case brave_history_embeddings::ComputeAnswerStatus::kExecutionCancelled:
    case brave_history_embeddings::ComputeAnswerStatus::kFiltered:
      return true;
  }
}
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

  if (input.omit_asynchronous_matches())
    return;

  if (!client()->IsHistoryEmbeddingsEnabled()) {
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

  brave_history_embeddings::HistoryEmbeddingsSearch* search =
      client()->GetHistoryEmbeddingsSearch();
  CHECK(search);
  done_ = false;
  client()->GetOmniboxTriggeredFeatureService()->FeatureTriggered(
      metrics::OmniboxEventProto_Feature_HISTORY_EMBEDDINGS_FEATURE);
  search->Search(
      nullptr, base::UTF16ToUTF8(input_.text()), {}, provider_max_matches_,
      /*skip_answering=*/false, /*url_id_filter=*/{},
      base::BindRepeating(&BraveHistoryEmbeddingsProvider::OnReceivedSearchResult,
                          weak_factory_.GetWeakPtr()));
}

void BraveHistoryEmbeddingsProvider::Stop(AutocompleteStopReason stop_reason) {
  // TODO(crbug.com/364303536): Ignore the stop timer since we know answers take
  //   longer than 1500ms to generate.
  if (stop_reason == AutocompleteStopReason::kInactivity) {
    return;
  }

  // Erase the abandoned placeholder answer.
  if (!done_) {
    size_t erased_count = std::erase_if(matches_, [&](const auto& match) {
      return match.type ==
             omnibox::AutocompleteMatchType::kHistoryEmbeddingsAnswer;
    });
    CHECK_LE(erased_count, 1u);
    if (erased_count)
      NotifyListeners(!matches_.empty());
  }

  AutocompleteProvider::Stop(stop_reason);

  // TODO(b/333770460): Once `HistoryEmbeddingsService` has a stop API, we
  //   should call it here.
}

void BraveHistoryEmbeddingsProvider::OnReceivedSearchResult(
    brave_history_embeddings::SearchResult search_result) {
  // Check `done_` in case the stop timer fired or the user closed the omnibox
  // before `Search()` completed. Check `last_search_input_` in case this is the
  // result for an earlier `Search()` request; there's usually 2 requests
  // ongoing as the user types.
  if (done_ || search_result.query != base::UTF16ToUTF8(input_.text()))
    return;

  // `OnReceivedSearchResult()` can be called multiple times per `Search()`
  // request. Clear `matches_``to avoid aggregating duplicates.
  matches_.clear();

  if (search_result.scored_url_rows.empty())
    return;

  for (const brave_history_embeddings::ScoredUrlRow& scored_url_row :
       search_result.scored_url_rows) {
    matches_.push_back(CreateMatch(scored_url_row));
  }

  bool answers_enabled =
      brave_history_embeddings::GetFeatureParameters().answers_in_omnibox_scoped &&
      input_.in_keyword_mode();
  if (answers_enabled) {
    auto optional_match = CreateAnswerMatch(
        search_result.answerer_result,
        search_result.scored_url_rows[search_result.AnswerIndex()],
        matches_[search_result.AnswerIndex()]);
    if (optional_match)
      matches_.push_back(optional_match.value());
  }

  done_ =
      !answers_enabled || IsAnswerDone(search_result.answerer_result.status);
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

std::optional<AutocompleteMatch> BraveHistoryEmbeddingsProvider::CreateAnswerMatch(
    const brave_history_embeddings::AnswererResult& answerer_result,
    const brave_history_embeddings::ScoredUrlRow& scored_url_row,
    const AutocompleteMatch& match) {
  // If the match is outscored and not shown, then the answer shouldn't show
  // either.
  int score = std::max(match.relevance - 1, 1);

  switch (answerer_result.status) {
    case brave_history_embeddings::ComputeAnswerStatus::kUnspecified:
    case brave_history_embeddings::ComputeAnswerStatus::kUnanswerable:
    case brave_history_embeddings::ComputeAnswerStatus::kFiltered:
    case brave_history_embeddings::ComputeAnswerStatus::kExecutionCancelled:
    case brave_history_embeddings::ComputeAnswerStatus::kModelUnavailable:
      return std::nullopt;

    case brave_history_embeddings::ComputeAnswerStatus::kLoading: {
      AutocompleteMatch answer_match = CreateAnswerMatchHelper(
          score,
          l10n_util::GetStringUTF16(
              IDS_HISTORY_EMBEDDINGS_ANSWER_LOADING_HEADING),
          u"");
      answer_match.history_embeddings_answer_header_loading = true;
      return answer_match;
    }

    case brave_history_embeddings::ComputeAnswerStatus::kSuccess: {
      AutocompleteMatch answer_match = CreateAnswerMatchHelper(
          score,
          l10n_util::GetStringUTF16(IDS_HISTORY_EMBEDDINGS_ANSWER_HEADING),
          AutocompleteMatch::SanitizeString(
              base::UTF8ToUTF16(answerer_result.answer.text())));
      answer_match.destination_url =
          GURL{"chrome://history/?q=" + answerer_result.query};
      std::u16string source = history_clusters::ComputeURLForDisplay(
          scored_url_row.row.url(), brave_history_embeddings::GetFeatureParameters()
                                        .trim_after_host_in_results);
      answer_match.contents = AutocompleteMatch::SanitizeString(
          source + u"  •  " +
          l10n_util::GetStringFUTF16(
              IDS_HISTORY_EMBEDDINGS_ANSWER_SOURCE_VISIT_DATE_LABEL,
              base::TimeFormatShortDate(scored_url_row.row.last_visit())));
      answer_match.contents_class = {{0, ACMatchClassification::DIM}};
      return answer_match;
    }

    case brave_history_embeddings::ComputeAnswerStatus::kExecutionFailure:
      return CreateAnswerMatchHelper(
          score,
          l10n_util::GetStringUTF16(IDS_HISTORY_EMBEDDINGS_ANSWER_HEADING),
          l10n_util::GetStringUTF16(
              IDS_HISTORY_EMBEDDINGS_ANSWERER_ERROR_TRY_AGAIN));
  }
}

AutocompleteMatch BraveHistoryEmbeddingsProvider::CreateAnswerMatchHelper(
    int score,
    const std::u16string& history_embeddings_answer_header_text,
    const std::u16string& description) {
  AutocompleteMatch match(
      this, score, /*deletable=*/false,
      omnibox::AutocompleteMatchType::kHistoryEmbeddingsAnswer);
  match.history_embeddings_answer_header_text =
      history_embeddings_answer_header_text;
  match.description = description;
  if (!description.empty())
    match.description_class = {{0, ACMatchClassification::NONE}};
  match.RecordAdditionalInfo("history_embeddings_answer_header_text",
                             history_embeddings_answer_header_text);
  if (starter_pack_engine_) {
    match.keyword = starter_pack_engine_->keyword();
    match.transition = ui::PAGE_TRANSITION_KEYWORD;
  }
  return match;
}
