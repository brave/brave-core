/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/history_embeddings/history_embeddings_service_factory.h"

#include "brave/components/history_embeddings/content/brave_history_embeddings_service.h"
#include "chrome/browser/history_embeddings/chrome_history_embeddings_service.h"

// Route through our template so OnPassageVisibilityCalculated synthesizes a
// passing visibility score — Brave doesn't use PageContentAnnotationsService
// for content visibility filtering.
#define ChromeHistoryEmbeddingsService \
  BraveHistoryEmbeddingsService<       \
      history_embeddings::ChromeHistoryEmbeddingsService>

#include <chrome/browser/history_embeddings/history_embeddings_service_factory.cc>

#undef ChromeHistoryEmbeddingsService
