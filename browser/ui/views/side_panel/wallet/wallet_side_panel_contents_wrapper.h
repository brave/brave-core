/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_WALLET_WALLET_SIDE_PANEL_CONTENTS_WRAPPER_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_WALLET_WALLET_SIDE_PANEL_CONTENTS_WRAPPER_H_

#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/webui/top_chrome/webui_contents_wrapper.h"

class Profile;
class WalletPanelUI;

// Custom WebUIContentsWrapper for the wallet side panel that disables
// auto-resize. WalletPanelUI's config has ShouldAutoResizeHost() = true
// (needed for the popup bubble), but in the side panel context we want
// the content to fill the available space.
class WalletSidePanelContentsWrapper : public WebUIContentsWrapper {
 public:
  WalletSidePanelContentsWrapper(const GURL& webui_url,
                                 Profile* profile,
                                 int task_manager_string_id,
                                 bool esc_closes_ui);
  ~WalletSidePanelContentsWrapper() override;

  // WebUIContentsWrapper:
  void ReloadWebContents() override;
  base::WeakPtr<WebUIContentsWrapper> GetWeakPtr() override;

  WalletPanelUI* GetWebUIController();

 private:
  const GURL webui_url_;
  base::WeakPtrFactory<WebUIContentsWrapper> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_WALLET_WALLET_SIDE_PANEL_CONTENTS_WRAPPER_H_
