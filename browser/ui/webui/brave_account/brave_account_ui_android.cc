/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_account/brave_account_ui_android.h"

#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "brave/browser/brave_account/dialog_mode_holder.h"
#include "brave/browser/ui/brave_account/brave_account_dialog_opener.h"
#include "brave/components/brave_account/features.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/common/url_constants.h"
#include "ui/webui/webui_util.h"

BraveAccountUIAndroid::BraveAccountUIAndroid(content::WebUI* web_ui)
    : BraveAccountUIBase(Profile::FromWebUI(web_ui),
                         web_ui->GetWebContents()->GetVisibleURL(),
                         base::BindOnce(&webui::SetupWebUIDataSource)),
      WebUIController(web_ui) {}

BraveAccountUIAndroid::~BraveAccountUIAndroid() = default;

void BraveAccountUIAndroid::BindInterface(
    mojo::PendingReceiver<brave_account::mojom::DialogController>
        pending_receiver) {
  dialog_controller_receiver_.reset();
  dialog_controller_receiver_.Bind(std::move(pending_receiver));
}

void BraveAccountUIAndroid::BindInterface(
    mojo::PendingReceiver<brave_account::mojom::DialogOpener>
        pending_receiver) {
  dialog_opener_receiver_.reset();
  dialog_opener_receiver_.Bind(std::move(pending_receiver));
}

void BraveAccountUIAndroid::CloseDialog() {
  web_ui()->GetWebContents()->Close();
}

void BraveAccountUIAndroid::GetDialogMode(GetDialogModeCallback callback) {
  std::move(callback).Run(brave_account::DialogModeHolder::GetDialogMode(
      CHECK_DEREF(web_ui()->GetWebContents())));
}

void BraveAccountUIAndroid::OpenDialog(
    const std::string& initiating_service_name,
    brave_account::mojom::DialogMode dialog_mode) {
  brave_account::OpenBraveAccountDialog(CHECK_DEREF(web_ui()->GetWebContents()),
                                        initiating_service_name, dialog_mode);
}

WEB_UI_CONTROLLER_TYPE_IMPL(BraveAccountUIAndroid)

BraveAccountUIAndroidConfig::BraveAccountUIAndroidConfig()
    : DefaultWebUIConfig(content::kChromeUIScheme, kBraveAccountHost) {
  CHECK(brave_account::features::IsBraveAccountEnabled());
}
