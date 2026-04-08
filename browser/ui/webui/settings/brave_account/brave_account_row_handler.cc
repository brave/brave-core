/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_account/brave_account_row_handler.h"

#include "brave/browser/ui/webui/brave_account/brave_account_ui_desktop.h"

namespace brave_account {

BraveAccountRowHandler::BraveAccountRowHandler(content::WebUI* web_ui)
    : web_ui_(web_ui) {}

BraveAccountRowHandler::~BraveAccountRowHandler() = default;

void BraveAccountRowHandler::OpenDialog(
    const std::string& initiating_service_name) {
  ShowBraveAccountDialog(web_ui_, initiating_service_name);
}

}  // namespace brave_account
