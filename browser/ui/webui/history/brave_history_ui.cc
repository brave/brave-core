/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/history/brave_history_ui.h"

#include "brave/browser/ui/webui/history/brave_history_embeddings_page_handler.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"

BraveHistoryUI::BraveHistoryUI(content::WebUI* web_ui) : HistoryUI(web_ui) {}

BraveHistoryUI::~BraveHistoryUI() = default;

void BraveHistoryUI::BindInterface(
    mojo::PendingReceiver<brave_history_embeddings::mojom::PageHandlerFactory>
        receiver) {
  page_handler_factory_receiver_.reset();
  page_handler_factory_receiver_.Bind(std::move(receiver));
}

void BraveHistoryUI::CreateInterfacePageHandler(
    mojo::PendingRemote<brave_history_embeddings::mojom::Page> page,
    mojo::PendingReceiver<brave_history_embeddings::mojom::PageHandler>
        receiver) {
  page_handler_ = std::make_unique<BraveHistoryEmbeddingsPageHandler>(
      std::move(receiver), std::move(page),
      Profile::FromWebUI(web_ui())->GetPrefs());
}
