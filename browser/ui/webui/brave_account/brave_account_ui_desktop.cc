/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_account/brave_account_ui_desktop.h"

#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "brave/browser/brave_account/dialog_mode_holder.h"
#include "brave/components/brave_account/features.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/common/url_constants.h"
#include "ui/web_dialogs/web_dialog_delegate.h"
#include "ui/webui/webui_util.h"

BraveAccountUIDesktop::BraveAccountUIDesktop(content::WebUI* web_ui)
    : BraveAccountUIBase(Profile::FromWebUI(web_ui),
                         web_ui->GetWebContents()->GetVisibleURL(),
                         base::BindOnce(&webui::SetupWebUIDataSource)),
      ConstrainedWebDialogUI(web_ui) {}

BraveAccountUIDesktop::~BraveAccountUIDesktop() = default;

void BraveAccountUIDesktop::BindInterface(
    mojo::PendingReceiver<brave_account::mojom::DialogController>
        pending_receiver) {
  receiver_.reset();
  receiver_.Bind(std::move(pending_receiver));
}

void BraveAccountUIDesktop::CloseDialog() {
  auto* constrained_delegate = GetConstrainedDelegate();
  auto* web_dialog_delegate = constrained_delegate
                                  ? constrained_delegate->GetWebDialogDelegate()
                                  : nullptr;
  if (!web_dialog_delegate) {
    return;
  }

  web_dialog_delegate->OnDialogClosed("");
  constrained_delegate->OnDialogCloseFromWebUI();
}

void BraveAccountUIDesktop::GetDialogMode(GetDialogModeCallback callback) {
  std::move(callback).Run(brave_account::DialogModeHolder::GetDialogMode(
      CHECK_DEREF(web_ui()->GetWebContents())));
}

WEB_UI_CONTROLLER_TYPE_IMPL(BraveAccountUIDesktop)

BraveAccountUIDesktopConfig::BraveAccountUIDesktopConfig()
    : DefaultWebUIConfig(content::kChromeUIScheme, kBraveAccountHost) {
  CHECK(brave_account::features::IsBraveAccountEnabled());
}
