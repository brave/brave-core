/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Point `enableHistoryEmbeddings` at the setting the embedding services were
// built with rather than the live pref upstream reads, so the semantic search
// input shows up only once there is an index behind it. The setting takes
// effect on relaunch. Hooked via macro substitution of the one-and-only
// ManagedUIHandler::Initialize() call in the upstream constructor -- by the
// time it runs the data source has been created and is in scope.

#include "brave/components/local_ai/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/managed_ui_handler.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"

#if BUILDFLAG(ENABLE_LOCAL_AI)
// Implemented in //brave/browser/history_embeddings:status. Returns the setting
// the profile's embedding services were built with. Forward declared to keep
// this override free of a compile-time dependency on //brave.
bool BraveHistoryEmbeddingsEnabledAtStartup(Profile* profile);
#endif  // BUILDFLAG(ENABLE_LOCAL_AI)

namespace {

class BraveHistoryUIInitializer {
 public:
  static void Initialize(content::WebUI* web_ui,
                         content::WebUIDataSource* source) {
    ManagedUIHandler::Initialize(web_ui, source);

#if BUILDFLAG(ENABLE_LOCAL_AI)
    source->AddBoolean(
        "enableHistoryEmbeddings",
        BraveHistoryEmbeddingsEnabledAtStartup(Profile::FromWebUI(web_ui)));
#endif  // BUILDFLAG(ENABLE_LOCAL_AI)
  }
};

}  // namespace

#define ManagedUIHandler BraveHistoryUIInitializer

#include <chrome/browser/ui/webui/history/history_ui.cc>

#undef ManagedUIHandler
