/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "services/passage_embeddings/passage_embeddings_op_resolver.h"

namespace passage_embeddings {

// Base class BuiltinOpResolver registers all TFLite ops and includes the
// XNNPACK delegate for CPU acceleration. This gives us full op coverage for
// our model of choice. Upstream uses the limited TFLiteOpResolver + optional
// GPU delegate via BUILD_WITH_ML_INTERNAL which we don't have as downstream.
PassageEmbeddingsOpResolver::PassageEmbeddingsOpResolver(
    bool allow_gpu_execution) {}

}  // namespace passage_embeddings
