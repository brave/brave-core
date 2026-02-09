/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_account/brave_account_settings_handler.h"

#include <utility>

#include "brave/browser/ui/webui/settings/brave_account/brave_account_row_handler.h"

namespace brave_account {

BraveAccountSettingsHandler::BraveAccountSettingsHandler(
    content::WebUI* web_ui,
    mojo::PendingReceiver<mojom::RowHandlerFactory> pending_receiver)
    : web_ui_(web_ui), receiver_(this, std::move(pending_receiver)) {}

BraveAccountSettingsHandler::~BraveAccountSettingsHandler() = default;

void BraveAccountSettingsHandler::CreateRowHandler(
    mojo::PendingReceiver<mojom::RowHandler> row_handler,
    mojo::PendingRemote<mojom::RowClient> row_client) {
  row_handler_ = std::make_unique<BraveAccountRowHandler>(
      std::move(row_handler), std::move(row_client), web_ui_);
}

}  // namespace brave_account
