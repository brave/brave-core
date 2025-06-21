/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_account_settings_handler.h"

#include <utility>

#include "brave/browser/ui/webui/brave_account/brave_account_ui_desktop.h"

namespace brave_account {
BraveAccountSettingsHandler::BraveAccountSettingsHandler(
    mojo::PendingReceiver<mojom::BraveAccountSettingsHandler> handler,
    content::WebUI* web_ui)
    : handler_(this, std::move(handler)), web_ui_(web_ui) {}

BraveAccountSettingsHandler::~BraveAccountSettingsHandler() = default;

void BraveAccountSettingsHandler::OpenDialog() {
  ShowBraveAccountDialog(web_ui_);
}
}  // namespace brave_account
