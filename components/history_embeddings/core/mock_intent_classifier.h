// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CORE_MOCK_INTENT_CLASSIFIER_H_
#define BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CORE_MOCK_INTENT_CLASSIFIER_H_

#include "brave/components/history_embeddings/core/intent_classifier.h"

namespace brave_history_embeddings {

class MockIntentClassifier : public IntentClassifier {
 public:
  MockIntentClassifier();
  ~MockIntentClassifier() override;

  // IntentClassifier:
  int64_t GetModelVersion() override;
  void ComputeQueryIntent(std::string query,
                          ComputeQueryIntentCallback callback) override;
};

}  // namespace brave_history_embeddings

#endif  // BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CORE_MOCK_INTENT_CLASSIFIER_H_
