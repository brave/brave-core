/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/files/file.h"
#include "services/passage_embeddings/passage_embedder_executor.h"

namespace brave_history_embeddings {

// Null implementation for builds without local AI (Android, Brave Origin). The
// override calls the hook unconditionally, so a definition must always link.
std::unique_ptr<passage_embeddings::PassageEmbedderExecutor>
MaybeCreateLitertExecutor(base::File&, int, int, int, int) {
  return nullptr;
}

}  // namespace brave_history_embeddings
