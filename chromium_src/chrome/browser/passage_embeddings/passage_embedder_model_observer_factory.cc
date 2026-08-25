/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Gate the passage embedder model observer on the per-profile history
// embeddings toggle. Both embedding services (PageEmbeddingsService,
// HistoryEmbeddingsService) refuse to build without this observer, so this
// keeps the on-device AI embedder from being created when the toggle is off.
// Applied at service creation, so a toggle change takes effect on restart.

#include "brave/components/local_ai/buildflags/buildflags.h"
#include "chrome/browser/history_embeddings/history_embeddings_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"

// Implemented in //brave/browser/passage_embeddings:chromium_impl. Returns the
// setting the profile's embedding services were built with.
bool BraveIsHistoryEmbeddingsEnabled(Profile* profile);

namespace history_embeddings {

// Per-profile overload the macro below routes the upstream no-arg call to,
// using the `context` in scope at the call site.
bool IsHistoryEmbeddingsFeatureEnabled(content::BrowserContext* context) {
  Profile* profile = Profile::FromBrowserContext(context);
#if BUILDFLAG(ENABLE_LOCAL_AI)
  return BraveIsHistoryEmbeddingsEnabled(profile);
#else
  return IsHistoryEmbeddingsEnabledForProfile(profile);
#endif  // BUILDFLAG(ENABLE_LOCAL_AI)
}

}  // namespace history_embeddings

#define IsHistoryEmbeddingsFeatureEnabled() \
  IsHistoryEmbeddingsFeatureEnabled(context)

#include <chrome/browser/passage_embeddings/passage_embedder_model_observer_factory.cc>

#undef IsHistoryEmbeddingsFeatureEnabled
