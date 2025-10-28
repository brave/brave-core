// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/email_aliases/email_aliases_panel_handler.h"

#include "brave/browser/ui/email_aliases/email_aliases_controller.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_web_ui_controller.h"
#include "chrome/browser/ui/webui/webui_embedding_context.h"

EmailAliasesPanelHandler::EmailAliasesPanelHandler(
    TopChromeWebUIController* webui_controller,
    mojo::PendingReceiver<email_aliases::mojom::EmailAliasesPanelHandler>
        receiver)
    : webui_controller_(webui_controller),
      receiver_(this, std::move(receiver)) {}

EmailAliasesPanelHandler::~EmailAliasesPanelHandler() = default;

email_aliases::EmailAliasesController*
EmailAliasesPanelHandler::GetEmailAliasesController() {
  if (auto* brower_windows_interface = webui::GetBrowserWindowInterface(
          webui_controller_->web_ui()->GetWebContents())) {
    return brower_windows_interface->GetFeatures().email_aliases_controller();
  }
  return nullptr;
}

void EmailAliasesPanelHandler::OnAliasCreationComplete(
    const std::string& email) {
  if (auto* controller = GetEmailAliasesController()) {
    controller->OnAliasCreationComplete(email);
  }
}

void EmailAliasesPanelHandler::OnManageAliases() {
  if (auto* controller = GetEmailAliasesController()) {
    controller->OpenSettingsPage();
  }
}

void EmailAliasesPanelHandler::OnCancel() {
  if (auto* controller = GetEmailAliasesController()) {
    controller->CloseBubble();
  }
}
