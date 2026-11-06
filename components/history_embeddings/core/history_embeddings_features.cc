// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/components/history_embeddings/core/history_embeddings_features.h"

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "build/build_config.h"

namespace brave_history_embeddings {

namespace {

FeatureParameters& GetFeatureParametersMutable() {
  static FeatureParameters parameters(true);
  return parameters;
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////

const base::FeatureParam<bool> kShowSourcePassages(&kHistoryEmbeddings,
                                                   "ShowSourcePassages",
                                                   false);

const base::FeatureParam<int> kPassageExtractionDelay(&kHistoryEmbeddings,
                                                      "PassageExtractionDelay",
                                                      5000);

const base::FeatureParam<int> kPassageExtractionMaxWordsPerAggregatePassage(
    &kHistoryEmbeddings,
    "PassageExtractionMaxWordsPerAggregatePassage",
    200);

const base::FeatureParam<int> kSearchQueryMinimumWordCount(
    &kHistoryEmbeddings,
    "SearchQueryMinimumWordCount",
    2);
const base::FeatureParam<int> kSearchPassageMinimumWordCount(
    &kHistoryEmbeddings,
    "SearchPassageMinimumWordCount",
    5);

// TODO(b/352384806): Take model metadata from Answerer when available,
//  and eliminate this parameter as it will then be unnecessary.
const base::FeatureParam<int> kContextPassagesMinimumWordCount(
    &kHistoryEmbeddings,
    "ContextPassagesMinimumWordCount",
    1000);

const base::FeatureParam<int> kSearchResultItemCount(&kHistoryEmbeddings,
                                                     "SearchResultItemCount",
                                                     3);

const base::FeatureParam<double> kContentVisibilityThreshold(
    &kHistoryEmbeddings,
    "ContentVisibilityThreshold",
    0);

// See comment at `brave_history_embeddings::GetScoreThreshold()`.
const base::FeatureParam<double> kSearchScoreThreshold(&kHistoryEmbeddings,
                                                       "SearchScoreThreshold",
                                                       -1);

const base::FeatureParam<double> kSearchWordMatchScoreThreshold(
    &kHistoryEmbeddings,
    "SearchWordMatchScoreThreshold",
    0.4);

// This one defaults true because we generally want it whenever v2 is enabled
// and it will only be used if applicable.
const base::FeatureParam<bool> kEnableIntentClassifier(&kHistoryEmbeddings,
                                                       "EnableIntentClassifier",
                                                       true);

const base::FeatureParam<int> kMockIntentClassifierDelayMS(
    &kHistoryEmbeddings,
    "MockIntentClassifierDelayMS",
    0);

const base::FeatureParam<int> kMockAnswererDelayMS(&kHistoryEmbeddings,
                                                   "MockAnswererDelayMS",
                                                   0);

// Default to `ComputeAnswerStatus::SUCCESS`.
const base::FeatureParam<int> kMockAnswererStatus(&kHistoryEmbeddings,
                                                  "MockAnswererStatus",
                                                  2);

const base::FeatureParam<bool> kEnableImagesForResults(&kHistoryEmbeddings,
                                                       "EnableImagesForResults",
                                                       true);

const base::FeatureParam<bool> kOmniboxScoped(&kHistoryEmbeddings,
                                              "OmniboxScoped",
                                              true);

const base::FeatureParam<bool> kOmniboxUnscoped(&kHistoryEmbeddings,
                                                "OmniboxUnscoped",
                                                false);

const base::FeatureParam<bool> kAnswersInOmniboxScoped(&kHistoryEmbeddings,
                                                       "AnswersInOmniboxScoped",
                                                       true);

const base::FeatureParam<bool> kSendQualityLog(&kHistoryEmbeddings,
                                               "SendQualityLog",
                                               false);
const base::FeatureParam<bool> kSendQualityLogV2(&kHistoryEmbeddings,
                                                 "SendQualityLogV2",
                                                 false);

const base::FeatureParam<bool> kDeleteEmbeddings(&kHistoryEmbeddings,
                                                 "DeleteEmbeddings",
                                                 false);
const base::FeatureParam<bool> kRebuildEmbeddings(&kHistoryEmbeddings,
                                                  "RebuildEmbeddings",
                                                  true);

const base::FeatureParam<bool> kUseUrlFilter(&kHistoryEmbeddings,
                                             "UseUrlFilter",
                                             false);

const base::FeatureParam<bool> kEnableSidePanel(&kHistoryEmbeddings,
                                                "EnableSidePanel",
                                                true);

const base::FeatureParam<bool> kTrimAfterHostInResults(&kHistoryEmbeddings,
                                                       "TrimAfterHostInResults",
                                                       true);

const base::FeatureParam<int> kMaxAnswererContextUrlCount(
    &kHistoryEmbeddings,
    "MaxAnswererContextUrlCount",
    1);

const base::FeatureParam<double> kWordMatchMinEmbeddingScore(
    &kHistoryEmbeddings,
    "WordMatchMinEmbeddingScore",
    0.7);

const base::FeatureParam<int> kWordMatchMinTermLength(&kHistoryEmbeddings,
                                                      "WordMatchMinTermLength",
                                                      0);

const base::FeatureParam<double> kWordMatchScoreBoostFactor(
    &kHistoryEmbeddings,
    "WordMatchScoreBoostFactor",
    0.2);

const base::FeatureParam<int> kWordMatchLimit(&kHistoryEmbeddings,
                                              "WordMatchLimit",
                                              5);

const base::FeatureParam<int> kWordMatchSmoothingFactor(
    &kHistoryEmbeddings,
    "WordMatchSmoothingFactor",
    0);

const base::FeatureParam<int> kWordMatchMaxTermCount(&kHistoryEmbeddings,
                                                     "WordMatchMaxTermCount",
                                                     10);

const base::FeatureParam<double> kWordMatchRequiredTermRatio(
    &kHistoryEmbeddings,
    "WordMatchRequiredTermRatio",
    1.0);

const base::FeatureParam<bool> kScrollTagsEnabled(&kHistoryEmbeddings,
                                                  "ScrollTagsEnabled",
                                                  false);

const base::FeatureParam<bool> kEraseNonAsciiCharacters(
    &kHistoryEmbeddings,
    "EraseNonAsciiCharacters",
    false);

const base::FeatureParam<bool> kWordMatchSearchNonAsciiPassages(
    &kHistoryEmbeddings,
    "WordMatchSearchNonAsciiPassages",
    false);

FeatureParameters::FeatureParameters(bool load_finch) {
  if (!load_finch) {
    return;
  }
  show_source_passages = kShowSourcePassages.Get();
  passage_extraction_delay = kPassageExtractionDelay.Get();
  passage_extraction_max_words_per_aggregate_passage =
      kPassageExtractionMaxWordsPerAggregatePassage.Get();
  search_query_minimum_word_count = kSearchQueryMinimumWordCount.Get();
  search_passage_minimum_word_count = kSearchPassageMinimumWordCount.Get();
  context_passages_minimum_word_count = kContextPassagesMinimumWordCount.Get();
  search_result_item_count = kSearchResultItemCount.Get();
  content_visibility_threshold = kContentVisibilityThreshold.Get();
  search_score_threshold = kSearchScoreThreshold.Get();
  search_word_match_score_threshold = kSearchWordMatchScoreThreshold.Get();
  enable_intent_classifier = kEnableIntentClassifier.Get();
  mock_intent_classifier_delay_ms = kMockIntentClassifierDelayMS.Get();
  mock_answerer_delay_ms = kMockAnswererDelayMS.Get();
  mock_answerer_status = kMockAnswererStatus.Get();
  enable_images_for_results = kEnableImagesForResults.Get();
  omnibox_scoped = kOmniboxScoped.Get();
  omnibox_unscoped = kOmniboxUnscoped.Get();
  answers_in_omnibox_scoped = kAnswersInOmniboxScoped.Get();
  send_quality_log = kSendQualityLog.Get();
  send_quality_log_v2 = kSendQualityLogV2.Get();
  delete_embeddings = kDeleteEmbeddings.Get();
  rebuild_embeddings = kRebuildEmbeddings.Get();
  use_url_filter = kUseUrlFilter.Get();
  enable_side_panel = kEnableSidePanel.Get();
  trim_after_host_in_results = kTrimAfterHostInResults.Get();
  max_answerer_context_url_count = kMaxAnswererContextUrlCount.Get();
  word_match_min_embedding_score = kWordMatchMinEmbeddingScore.Get();
  word_match_min_term_length = kWordMatchMinTermLength.Get();
  word_match_score_boost_factor = kWordMatchScoreBoostFactor.Get();
  word_match_limit = kWordMatchLimit.Get();
  word_match_smoothing_factor = kWordMatchSmoothingFactor.Get();
  word_match_max_term_count = kWordMatchMaxTermCount.Get();
  word_match_required_term_ratio = kWordMatchRequiredTermRatio.Get();
  scroll_tags_enabled = kScrollTagsEnabled.Get();
  erase_non_ascii_characters = kEraseNonAsciiCharacters.Get();
  word_match_search_non_ascii_passages = kWordMatchSearchNonAsciiPassages.Get();
}

FeatureParameters::FeatureParameters(const FeatureParameters&) = default;
FeatureParameters::FeatureParameters(FeatureParameters&&) = default;
FeatureParameters& FeatureParameters::operator=(const FeatureParameters&) =
    default;
FeatureParameters& FeatureParameters::operator=(FeatureParameters&&) = default;

////////////////////////////////////////////////////////////////////////////////

int ScopedFeatureParametersForTesting::instance_count_ = 0;

ScopedFeatureParametersForTesting::ScopedFeatureParametersForTesting()
    : original_parameters_(GetFeatureParameters()) {}

ScopedFeatureParametersForTesting::ScopedFeatureParametersForTesting(
    base::OnceCallback<void(FeatureParameters&)> change_parameters)
    : original_parameters_(GetFeatureParameters()) {
  CHECK_EQ(instance_count_, 0) << "Use only one instance at a time to avoid "
                                  "possibility of A,B,~A,~B side effects.";
  instance_count_++;

  FeatureParameters changed = original_parameters_;
  std::move(change_parameters).Run(changed);
  GetFeatureParametersMutable() = changed;
}

ScopedFeatureParametersForTesting::~ScopedFeatureParametersForTesting() {
  GetFeatureParametersMutable() = original_parameters_;
  instance_count_--;
}

FeatureParameters& ScopedFeatureParametersForTesting::Get() {
  return GetFeatureParametersMutable();
}

////////////////////////////////////////////////////////////////////////////////

const FeatureParameters& GetFeatureParameters() {
  return GetFeatureParametersMutable();
}

void SetFeatureParametersForTesting(FeatureParameters parameters) {
  GetFeatureParametersMutable() = parameters;
}

}  // namespace brave_history_embeddings
