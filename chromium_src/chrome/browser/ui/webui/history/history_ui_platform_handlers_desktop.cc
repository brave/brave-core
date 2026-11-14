/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Keep upstream's semantic search input off brave://history and add what the
// brave-history-embeddings element needs instead. Upstream's
// HistoryEmbeddingsService is never built in Brave, so the input it would show
// has no index behind it. Hooked via macro substitution of the one-and-only
// ManagedUIHandler::Initialize() call in the upstream constructor -- by the
// time it runs the data source has been created and is in scope.

#include "brave/components/local_ai/buildflags/buildflags.h"
#include "chrome/browser/ui/webui/managed_ui_handler.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"

#if BUILDFLAG(ENABLE_LOCAL_AI)
// Implemented in //brave/browser/ui/webui/history_embeddings. Forward declared
// to keep this override free of a compile-time dependency on //brave.
void BravePopulateHistoryEmbeddingsSource(content::WebUI* web_ui,
                                          content::WebUIDataSource* source);
#endif  // BUILDFLAG(ENABLE_LOCAL_AI)

namespace {

class BraveHistoryUIInitializer {
 public:
  static void Initialize(content::WebUI* web_ui,
                         content::WebUIDataSource* source) {
    ManagedUIHandler::Initialize(web_ui, source);
    source->AddBoolean("enableHistoryEmbeddings", false);
#if BUILDFLAG(ENABLE_LOCAL_AI)
    BravePopulateHistoryEmbeddingsSource(web_ui, source);
#endif  // BUILDFLAG(ENABLE_LOCAL_AI)
  }
};

}  // namespace

#define ManagedUIHandler BraveHistoryUIInitializer

#include <chrome/browser/ui/webui/history/history_ui_platform_handlers_desktop.cc>

#undef ManagedUIHandler
