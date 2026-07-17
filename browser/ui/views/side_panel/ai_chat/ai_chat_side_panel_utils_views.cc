// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <utility>

#include "base/feature_list.h"
#include "brave/browser/ui/side_panel/ai_chat/ai_chat_side_panel_utils.h"
#include "brave/browser/ui/views/side_panel/ai_chat/ai_chat_side_panel_tab_transfer_bridge.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "chrome/browser/tab_list/tab_list_interface.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"

namespace ai_chat {

Browser* GetBrowserForWebContents(content::WebContents* web_contents) {
  if (!web_contents) {
    return nullptr;
  }

  auto* browser_window =
      BrowserWindow::FindBrowserWindowWithWebContents(web_contents);
  auto* browser_view = static_cast<BrowserView*>(browser_window);
  if (!browser_view) {
    return nullptr;
  }

  return browser_view->browser();
}

void ClosePanel(content::WebContents* web_contents) {
  if (!web_contents) {
    return;
  }

  Browser* browser = GetBrowserForWebContents(web_contents);
  if (!browser) {
    return;
  }

  if (SidePanelUI* ui = browser->GetFeatures().side_panel_ui()) {
    ui->Close();
  }
}

void ClosePanelIfChatActive(content::WebContents* web_contents) {
  if (!web_contents) {
    return;
  }

  Browser* browser = GetBrowserForWebContents(web_contents);
  if (!browser) {
    return;
  }

  SidePanelUI* ui = browser->GetFeatures().side_panel_ui();
  if (ui && ui->GetCurrentEntryId() == SidePanelEntryId::kChatUI) {
    ui->Close();
  }
}

bool MaybeMoveFullPageChatToSidePanel(
    content::WebContents* ai_chat_web_contents) {
  if (!base::FeatureList::IsEnabled(features::kAIChatMoveFullPageToSidePanel)) {
    return false;
  }

  // Only move a full-page AI Chat. When the conversation is already hosted in
  // the side panel its contents is owned by the view and is not part of any tab
  // strip, so there is no owning tab and the click is left to the caller.
  tabs::TabInterface* ai_chat_tab =
      tabs::TabInterface::MaybeGetFromContents(ai_chat_web_contents);
  if (!ai_chat_tab) {
    return false;
  }

  BrowserWindowInterface* browser = ai_chat_tab->GetBrowserWindowInterface();
  if (!browser) {
    return false;
  }

  // The transfer only works with the global (window-scoped) side panel. A
  // tab-scoped side panel is tied to the tab that is active when it opens, so
  // once the moved conversation is shown and the clicked link activates a new
  // tab, the panel closes and the conversation would be destroyed. In
  // contextual mode leave the link to the caller: it opens in a tab and AI Chat
  // stays a full page.
  if (!ShouldSidePanelBeGlobal(browser->GetProfile())) {
    return false;
  }

  AIChatSidePanelTabTransferBridge* transfer_controller =
      browser->GetFeatures().ai_chat_side_panel_tab_transfer_bridge();
  if (!transfer_controller) {
    // Flag off, or a window type that has no controller.
    return false;
  }

  TabListInterface* tab_list = TabListInterface::From(browser);
  if (!tab_list) {
    return false;
  }

  const tabs::TabHandle ai_chat_handle = ai_chat_tab->GetHandle();

  // Detach AI Chat's live contents (not destroyed, not reloaded) and hand it to
  // the controller, which shows it in the side panel.
  std::unique_ptr<content::WebContents> ai_chat_contents =
      tab_list->DetachWebContents(ai_chat_handle);
  if (!ai_chat_contents) {
    return false;
  }

  transfer_controller->TransferFullPageContentsToSidePanel(
      std::move(ai_chat_contents));
  return true;
}

}  // namespace ai_chat
