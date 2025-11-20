/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/history_embeddings/history_embeddings_service_factory.h"

#include "brave/browser/history_embeddings/brave_passage_embeddings_service_controller.h"
#include "chrome/browser/passage_embeddings/chrome_passage_embeddings_service_controller.h"

// Replace Chrome's PassageEmbeddingsServiceController with Brave's
// Both are in the passage_embeddings namespace, so direct replacement works
#define ChromePassageEmbeddingsServiceController \
  BravePassageEmbeddingsServiceController

// Override GetEmbedder() call to return our BraveEmbedder for the given profile
// instead of base class's embedder_. This passes the profile to create
// per-profile embedders.
#define GetEmbedder() GetBraveEmbedder(profile)

#include <chrome/browser/history_embeddings/history_embeddings_service_factory.cc>

#undef GetEmbedder
#undef ChromePassageEmbeddingsServiceController
