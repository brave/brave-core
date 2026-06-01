/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/history/brave_history_embeddings_page_handler.h"

#include "base/functional/bind.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "components/prefs/pref_service.h"

BraveHistoryEmbeddingsPageHandler::BraveHistoryEmbeddingsPageHandler(
    mojo::PendingReceiver<brave_history_embeddings::mojom::PageHandler>
        receiver,
    mojo::PendingRemote<brave_history_embeddings::mojom::Page> page,
    PrefService* prefs)
    : receiver_(this, std::move(receiver)),
      page_(std::move(page)),
      prefs_(prefs) {
  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(
      local_ai::prefs::kBraveHistoryEmbeddingsEnabled,
      base::BindRepeating(&BraveHistoryEmbeddingsPageHandler::OnPrefChanged,
                          base::Unretained(this)));
  // Push the current pref value so the page reconciles with any change that
  // happened between the data source snapshot baked into loadTimeData and
  // this Mojo connection.
  OnPrefChanged();
}

BraveHistoryEmbeddingsPageHandler::~BraveHistoryEmbeddingsPageHandler() =
    default;

void BraveHistoryEmbeddingsPageHandler::SetEnabled(bool enabled) {
  prefs_->SetBoolean(local_ai::prefs::kBraveHistoryEmbeddingsEnabled, enabled);
}

void BraveHistoryEmbeddingsPageHandler::OnPrefChanged() {
  page_->OnEnabledChanged(
      prefs_->GetBoolean(local_ai::prefs::kBraveHistoryEmbeddingsEnabled));
}
