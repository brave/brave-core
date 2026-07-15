/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/passage_embeddings/chrome_passage_embeddings_service_controller.h"

#include "brave/browser/history_embeddings/brave_passage_embeddings_service_controller.h"
#include "components/passage_embeddings/core/passage_embeddings_service_controller.h"

// Replaces upstream's controller, which launches a sandboxed utility process,
// with Brave's in-process instance. Routing every caller through this one
// function avoids per-factory chromium_src overrides.

namespace passage_embeddings {

PassageEmbeddingsServiceController*
GetChromePassageEmbeddingsServiceController() {
  return BravePassageEmbeddingsServiceController::Get();
}

}  // namespace passage_embeddings
