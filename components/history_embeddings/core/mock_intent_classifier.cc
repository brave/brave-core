// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/components/history_embeddings/core/mock_intent_classifier.h"

#include "base/strings/string_util.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "brave/components/history_embeddings/core/history_embeddings_features.h"

namespace brave_history_embeddings {

MockIntentClassifier::MockIntentClassifier() = default;
MockIntentClassifier::~MockIntentClassifier() = default;

int64_t MockIntentClassifier::GetModelVersion() {
  return 1;
}

void MockIntentClassifier::ComputeQueryIntent(
    std::string query,
    ComputeQueryIntentCallback callback) {
  std::vector<std::string> query_intent_indicating_words({
      "can ",
      "could ",
      "do ",
      "does ",
      "how ",
      "should ",
      "what ",
      "when ",
      "where ",
      "whether ",
      "which ",
      "who ",
      "whose ",
      "why ",
      "would ",
  });
  query = base::ToLowerASCII(query);
  bool is_query_answerable =
      query.ends_with('?') ||
      std::ranges::any_of(
          query_intent_indicating_words,
          [&](std::string_view start) { return query.starts_with(start); });
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(std::move(callback), ComputeIntentStatus::SUCCESS,
                     is_query_answerable),
      base::Milliseconds(
          GetFeatureParameters().mock_intent_classifier_delay_ms));
}

}  // namespace brave_history_embeddings
