/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_account_handler.h"

#include <utility>

#include "brave/browser/ui/webui/brave_account/brave_account_dialogs_ui_desktop.h"

namespace brave_account {
BraveAccountHandler::BraveAccountHandler(
    mojo::PendingReceiver<mojom::BraveAccountHandler> handler,
    content::WebUI* web_ui)
    : handler_(this, std::move(handler)), web_ui_(web_ui) {}

BraveAccountHandler::~BraveAccountHandler() = default;

void BraveAccountHandler::OpenDialog() {
  ShowBraveAccountDialogs(web_ui_);
}
}  // namespace brave_account
