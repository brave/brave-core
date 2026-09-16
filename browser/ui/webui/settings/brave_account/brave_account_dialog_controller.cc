/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_account/brave_account_dialog_controller.h"

#include <utility>

#include "base/check_deref.h"
#include "brave/browser/ui/brave_account/brave_account_dialog_opener.h"
#include "content/public/browser/web_ui.h"

namespace brave_account {

BraveAccountDialogController::BraveAccountDialogController(
    content::WebUI& web_ui)
    : web_ui_(web_ui) {}

BraveAccountDialogController::~BraveAccountDialogController() = default;

void BraveAccountDialogController::OpenDialog(
    const std::string& initiating_service_name,
    mojom::DialogMode dialog_mode) {
  OpenBraveAccountDialog(CHECK_DEREF(web_ui_->GetWebContents()),
                         initiating_service_name, dialog_mode);
}

void BraveAccountDialogController::GetDialogMode(
    GetDialogModeCallback callback) {
  std::move(callback).Run(mojom::DialogMode::kDefault);
}

}  // namespace brave_account
