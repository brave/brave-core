// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CORE_HISTORY_EMBEDDINGS_FEATURES_H_
#define BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CORE_HISTORY_EMBEDDINGS_FEATURES_H_

#include "base/feature_list.h"
#include "base/functional/callback.h"
#include "base/metrics/field_trial_params.h"
#include "components/history_embeddings/core/history_embeddings_features.h"

namespace brave_history_embeddings {

// Upstream still defines these features. A second `BASE_FEATURE` with the
// same name would fail `FeatureList::CheckFeatureIdentity`.
using history_embeddings::kHistoryEmbeddings;
using history_embeddings::kHistoryEmbeddingsAnswers;

// Contains feature configuration state. Can be set using Finch or overridden
// for testing. Prefer to use this struct instead of feature parameters
// directly so as to reduce dependency on Finch.
struct FeatureParameters {
  explicit FeatureParameters(bool load_finch);
  FeatureParameters(const FeatureParameters&);
  FeatureParameters(FeatureParameters&&);
  FeatureParameters& operator=(const FeatureParameters&);
  FeatureParameters& operator=(FeatureParameters&&);

  // The minimum number of words a query or passage must have in order to be
  // included in similarity search.
  int search_query_minimum_word_count = 2;
  int search_passage_minimum_word_count = 5;

  // Specifies the similarity score threshold that embeddings must pass in order
  // for their results to be shown to the user. This is for general search
  // scoring and result inclusion.
  double search_score_threshold = -1;

  // Minimum word match score required to include an extra search result when
  // the total score does not meet the `search_score_threshold`.
  double search_word_match_score_threshold = 0.2;

  // Whether history embedding results should be shown in the omnibox when in
  // the '@history' scope.
  bool omnibox_scoped = true;

  // Whether history embedding results should be shown in the omnibox when not
  // in the '@history' scope. If true, behaves as if `kOmniboxScoped` is also
  // true.
  bool omnibox_unscoped = false;

  // These parameters control deletion and rebuilding of the embeddings
  // database. If `kDeleteEmbeddings` is true, the embeddings table will
  // be cleared on startup, effectively simulating a model version change.
  // If `kRebuildEmbeddings` is true (the default) then any rows in
  // the passages table without a corresponding row in the embeddings
  // table (keyed on url_id) will be queued for reprocessing by the embedder.
  bool delete_embeddings = false;
  bool rebuild_embeddings = true;

  // Specifies whether history embedding results should show just the hostname
  // of the result's URL.
  bool trim_after_host_in_results = true;

  // These control score boosting from passage text word matching.
  // See comments for `SearchParams` struct for more details about each value.
  double word_match_min_embedding_score = 0.7;
  int word_match_min_term_length = 0;
  double word_match_score_boost_factor = 0.2;
  int word_match_limit = 5;
  int word_match_smoothing_factor = 0;
  int word_match_max_term_count = 10;
  double word_match_required_term_ratio = 1.0;

  // Whether to erase non-ASCII characters from passages before sending them to
  // the ML embedder. When false, passages are embedded without modification but
  // then will be skipped during search. When true, passages are embedded with
  // non-ASCII characters removed, but are then included in search.
  bool erase_non_ascii_characters = false;

  // Whether to use word match text search for passages containing non-ASCII
  // characters. See also `word_match_min_embedding_score`, which this bypasses.
  // Note, when `erase_non_ascii_characters` is true, this will have no effect.
  bool word_match_search_non_ascii_passages = false;
};

// Use this to apply changes for testing only while an instance lives.
// Do not instantiate multiple since that could leak side effects.
// Instead, prefer to keep changes localized within block scopes.
class ScopedFeatureParametersForTesting {
 public:
  // Changes can be affected either by passing a lambda to constructor
  // or by modifying the instance returned by `Get`.
  ScopedFeatureParametersForTesting();
  explicit ScopedFeatureParametersForTesting(
      base::OnceCallback<void(FeatureParameters&)> change_parameters);
  ~ScopedFeatureParametersForTesting();

  // Returns mutable reference to static `FeatureParameters`, which can
  // be modified safely while this instance lives.
  FeatureParameters& Get();

 private:
  static int instance_count_;
  const FeatureParameters original_parameters_;
};

// Use this to get the feature parameter configuration. This is immutable
// when running the browser.
const FeatureParameters& GetFeatureParameters();

// Use this to set the feature parameter configuration. This can only be
// done in tests.
void SetFeatureParametersForTesting(FeatureParameters parameters);

}  // namespace brave_history_embeddings

#endif  // BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CORE_HISTORY_EMBEDDINGS_FEATURES_H_
