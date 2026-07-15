/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Gate the passage embedder model observer on the per-profile history
// embeddings toggle. Both embedding services (PageEmbeddingsService,
// HistoryEmbeddingsService) refuse to build without this observer, so this
// keeps the on-device AI embedder from being created when the toggle is off.
// Applied at service creation, so a toggle change takes effect on restart.

#include "chrome/browser/history_embeddings/history_embeddings_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"

namespace history_embeddings {

// Per-profile overload the macro below routes the upstream no-arg call to,
// using the `context` in scope at the call site.
bool IsHistoryEmbeddingsFeatureEnabled(content::BrowserContext* context) {
  return IsHistoryEmbeddingsEnabledForProfile(
      Profile::FromBrowserContext(context));
}

}  // namespace history_embeddings

#define IsHistoryEmbeddingsFeatureEnabled() \
  IsHistoryEmbeddingsFeatureEnabled(context)

#include <chrome/browser/passage_embeddings/passage_embedder_model_observer_factory.cc>

#undef IsHistoryEmbeddingsFeatureEnabled
