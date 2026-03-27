/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/history_embeddings/history_embeddings_service_factory.h"

#include "brave/browser/history_embeddings/brave_passage_embeddings_service_controller.h"
#include "brave/components/history_embeddings/content/brave_history_embeddings_service.h"
#include "chrome/browser/history_embeddings/chrome_history_embeddings_service.h"
#include "chrome/browser/passage_embeddings/chrome_passage_embeddings_service_controller.h"

// Replace Chrome's PassageEmbeddingsServiceController with Brave's
// Both are in the passage_embeddings namespace, so direct replacement works
#define ChromePassageEmbeddingsServiceController \
  BravePassageEmbeddingsServiceController

// Include the chrome header above so the class is declared before renaming.
// The #define only affects make_unique calls in the .cc body.
// See docs/gni_sources.md for the template pattern.
#define ChromeHistoryEmbeddingsService \
  BraveHistoryEmbeddingsService<       \
      history_embeddings::ChromeHistoryEmbeddingsService>

// Override GetEmbedder() to return the shared BraveEmbedder backed by
// the guest profile.
#define GetEmbedder() GetBraveEmbedder()

#include <chrome/browser/history_embeddings/history_embeddings_service_factory.cc>

#undef GetEmbedder
#undef ChromeHistoryEmbeddingsService
#undef ChromePassageEmbeddingsServiceController
