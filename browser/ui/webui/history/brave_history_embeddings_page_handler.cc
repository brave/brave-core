/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/history/brave_history_embeddings_page_handler.h"

#include "base/functional/bind.h"
#include "brave/browser/history_embeddings/brave_history_embeddings_status.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "chrome/browser/history_embeddings/history_embeddings_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"

BraveHistoryEmbeddingsPageHandler::BraveHistoryEmbeddingsPageHandler(
    mojo::PendingReceiver<brave_history_embeddings::mojom::PageHandler>
        receiver,
    mojo::PendingRemote<brave_history_embeddings::mojom::Page> page,
    Profile* profile,
    PrefService* local_state)
    : receiver_(this, std::move(receiver)),
      page_(std::move(page)),
      profile_(profile),
      local_state_(local_state) {
  pref_change_registrar_.Init(profile_->GetPrefs());
  pref_change_registrar_.Add(
      local_ai::prefs::kBraveHistoryEmbeddingsEnabled,
      base::BindRepeating(&BraveHistoryEmbeddingsPageHandler::OnPrefChanged,
                          base::Unretained(this)));
  local_state_change_registrar_.Init(local_state_);
  local_state_change_registrar_.Add(
      local_ai::prefs::kBraveLocalAIEnabled,
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
  profile_->GetPrefs()->SetBoolean(
      local_ai::prefs::kBraveHistoryEmbeddingsEnabled, enabled);
}

void BraveHistoryEmbeddingsPageHandler::OnPrefChanged() {
  // Same helper the WebUI data source reads `enableHistoryEmbeddings` from, so
  // the pushed value matches what the rest of the page sees.
  page_->OnEnabledChanged(
      history_embeddings::IsHistoryEmbeddingsEnabledForProfile(profile_),
      history_embeddings::BraveHistoryEmbeddingsStatus::GetForProfile(profile_)
          ->NeedsRestart());
}
