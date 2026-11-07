/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Gate the passage embedder model observer on the per-profile history
// embeddings toggle. Both embedding services (PageEmbeddingsService,
// HistoryEmbeddingsService) refuse to build without this observer, so this
// keeps the on-device AI embedder from being created when the toggle is off.
// Applied at service creation, so a toggle change takes effect on restart.
// The hook is inserted by
// rewrite/chrome/browser/passage_embeddings/
// passage_embedder_model_observer_factory.cc.yaml.

#include "brave/components/local_ai/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"

#if BUILDFLAG(ENABLE_LOCAL_AI)
// Implemented in //brave/browser/history_embeddings:status. Returns the setting
// the profile's embedding services were built with. Forward declared to keep
// this override free of a compile-time dependency on //brave.
bool BraveHistoryEmbeddingsEnabledAtStartup(Profile* profile);
#endif  // BUILDFLAG(ENABLE_LOCAL_AI)

namespace passage_embeddings {
namespace {

bool BraveShouldBuildPassageEmbedderModelObserver(
    content::BrowserContext* context) {
#if BUILDFLAG(ENABLE_LOCAL_AI)
  return BraveHistoryEmbeddingsEnabledAtStartup(
      Profile::FromBrowserContext(context));
#else
  return false;
#endif  // BUILDFLAG(ENABLE_LOCAL_AI)
}

}  // namespace
}  // namespace passage_embeddings

#include <chrome/browser/passage_embeddings/passage_embedder_model_observer_factory.cc>
