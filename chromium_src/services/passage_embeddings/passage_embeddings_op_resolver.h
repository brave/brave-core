/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_SERVICES_PASSAGE_EMBEDDINGS_PASSAGE_EMBEDDINGS_OP_RESOLVER_H_
#define BRAVE_CHROMIUM_SRC_SERVICES_PASSAGE_EMBEDDINGS_PASSAGE_EMBEDDINGS_OP_RESOLVER_H_

#include "third_party/tflite/src/tensorflow/lite/kernels/register.h"

namespace passage_embeddings {

class PassageEmbeddingsOpResolver
    : public tflite::ops::builtin::BuiltinOpResolver {
 public:
  explicit PassageEmbeddingsOpResolver(bool allow_gpu_execution);
};

}  // namespace passage_embeddings

#endif  // BRAVE_CHROMIUM_SRC_SERVICES_PASSAGE_EMBEDDINGS_PASSAGE_EMBEDDINGS_OP_RESOLVER_H_
