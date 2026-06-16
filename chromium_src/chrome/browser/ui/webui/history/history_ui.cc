/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Inject two loadTimeData entries (`isHistoryEmbeddingsFeatureEnabled` plus
// the toggle's localized strings) into the chrome://history WebUI data
// source. Hooked via macro substitution of the one-and-only
// ManagedUIHandler::Initialize() call in the upstream constructor — by the
// time it runs the data source has been created and is in scope. The
// Mojo interface for write/observer plumbing lives in BraveHistoryUI; this
// override only augments the data source.

#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/history_embeddings/history_embeddings_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/managed_ui_handler.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/base/webui/web_ui_util.h"

namespace {

class BraveHistoryUIInitializer {
 public:
  static void Initialize(content::WebUI* web_ui,
                         content::WebUIDataSource* source) {
    ManagedUIHandler::Initialize(web_ui, source);

    source->AddBoolean("isHistoryEmbeddingsFeatureEnabled",
                       history_embeddings::IsHistoryEmbeddingsFeatureEnabled());

    static constexpr webui::LocalizedString kBraveHistoryEmbeddingsStrings[] = {
        {"braveHistoryEmbeddingsToggleLabel",
         IDS_BRAVE_HISTORY_EMBEDDINGS_TOGGLE_LABEL},
        {"braveHistoryEmbeddingsToggleDescription",
         IDS_BRAVE_HISTORY_EMBEDDINGS_TOGGLE_DESCRIPTION},
    };
    source->AddLocalizedStrings(kBraveHistoryEmbeddingsStrings);
  }
};

}  // namespace

#define ManagedUIHandler BraveHistoryUIInitializer

#include <chrome/browser/ui/webui/history/history_ui.cc>

#undef ManagedUIHandler
