/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/wallet/wallet_side_panel_contents_wrapper.h"

#include "brave/browser/ui/webui/brave_wallet/wallet_panel_ui.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "ui/base/page_transition_types.h"

WalletSidePanelContentsWrapper::WalletSidePanelContentsWrapper(
    const GURL& webui_url,
    Profile* profile,
    int task_manager_string_id,
    bool esc_closes_ui)
    : WebUIContentsWrapper(webui_url,
                           profile,
                           task_manager_string_id,
                           // WalletPanelUIConfig::ShouldAutoResizeHost()
                           // returns true (needed for the popup bubble's
                           // preferred sizing). We pass false here so the
                           // panel stretches to fill the side panel width.
                           /*webui_resizes_host=*/false,
                           esc_closes_ui,
                           /*supports_draggable_regions=*/false,
                           WalletPanelUI::GetWebUIName()),
      webui_url_(webui_url) {
  CHECK(GetWebUIController());
  GetWebUIController()->set_embedder(weak_ptr_factory_.GetWeakPtr());
}

WalletSidePanelContentsWrapper::~WalletSidePanelContentsWrapper() = default;

void WalletSidePanelContentsWrapper::ReloadWebContents() {
  web_contents()->GetController().LoadURL(webui_url_, content::Referrer(),
                                          ui::PAGE_TRANSITION_AUTO_TOPLEVEL,
                                          std::string());
  // Re-bind embedder after navigation since LoadURL creates a fresh WebUI.
  if (auto* controller = GetWebUIController()) {
    controller->set_embedder(weak_ptr_factory_.GetWeakPtr());
  }
}

base::WeakPtr<WebUIContentsWrapper>
WalletSidePanelContentsWrapper::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

WalletPanelUI* WalletSidePanelContentsWrapper::GetWebUIController() {
  content::WebUI* const webui = web_contents()->GetWebUI();
  return webui && webui->GetController()
             ? webui->GetController()->GetAs<WalletPanelUI>()
             : nullptr;
}
