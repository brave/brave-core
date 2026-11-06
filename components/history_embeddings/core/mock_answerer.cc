// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/components/history_embeddings/core/mock_answerer.h"

#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "brave/components/history_embeddings/core/history_embeddings_features.h"
#include "components/optimization_guide/proto/features/history_answer.pb.h"

namespace brave_history_embeddings {

MockAnswerer::MockAnswerer() = default;
MockAnswerer::~MockAnswerer() = default;

int64_t MockAnswerer::GetModelVersion() {
  return 1;
}

void MockAnswerer::ComputeAnswer(std::string query,
                                 Context context,
                                 ComputeAnswerCallback callback) {
  optimization_guide::proto::Answer answer;
  answer.set_text(std::string("This is the answer to query '") + query +
                  std::string("'."));
  AnswererResult result(static_cast<ComputeAnswerStatus>(
                            GetFeatureParameters().mock_answerer_status),
                        query, answer, {}, "url.com", {});

  // Set URL and passage citation if available.
  auto it = context.url_passages_map.begin();
  if (it != context.url_passages_map.end()) {
    optimization_guide::proto::Citation* citation =
        result.answer.add_citations();
    citation->set_passage_id("0001");
    result.url = it->first;
    result.PopulateScrollToTextFragment(it->second);
  }

  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE, base::BindOnce(std::move(callback), std::move(result)),
      base::Milliseconds(GetFeatureParameters().mock_answerer_delay_ms));
}

}  // namespace brave_history_embeddings
