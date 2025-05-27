/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_account_handler.h"

#include <utility>

#include "brave/browser/ui/webui/brave_account/brave_account_dialogs_ui.h"

namespace brave_account {
BraveAccountHandler::BraveAccountHandler(
    content::WebUI* web_ui,
    mojo::PendingReceiver<mojom::BraveAccountHandler> handler)
    : web_ui_(web_ui), handler_(this, std::move(handler)) {}

BraveAccountHandler::~BraveAccountHandler() = default;

void BraveAccountHandler::OpenDialog() {
  BraveAccountDialogsDialog::Show(web_ui_);
}
}  // namespace brave_account
