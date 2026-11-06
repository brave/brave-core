// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CORE_INTENT_CLASSIFIER_H_
#define BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CORE_INTENT_CLASSIFIER_H_

#include <string>
#include <vector>

#include "base/functional/callback.h"

namespace brave_history_embeddings {

// The status of an intent classification attempt.
enum class ComputeIntentStatus {
  // Not yet specified. This status means the intent isn't classified yet.
  UNSPECIFIED,

  // Intent classified successfully.
  SUCCESS,

  // The model files required for generation are not available.
  MODEL_UNAVAILABLE,

  // Failure occurred during model execution.
  EXECUTION_FAILURE,

  // Model execution cancelled.
  EXECUTION_CANCELLED,
};

// Callback type to receive intent classification result.
using ComputeQueryIntentCallback =
    base::OnceCallback<void(ComputeIntentStatus status,
                            bool is_query_answerable)>;

// Base class that hides implementation details for how intents are classified.
class IntentClassifier {
 public:
  virtual ~IntentClassifier() = default;

  // Returns 0 if not ready, and the nonzero model version number when it's
  // loaded and ready.
  virtual int64_t GetModelVersion() = 0;

  // Calls `callback` asynchronously with intent classification result.
  virtual void ComputeQueryIntent(std::string query,
                                  ComputeQueryIntentCallback callback) = 0;
};

}  // namespace brave_history_embeddings

#endif  // BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CORE_INTENT_CLASSIFIER_H_
