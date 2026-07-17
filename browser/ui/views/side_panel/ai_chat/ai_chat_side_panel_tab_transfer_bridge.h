// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_AI_CHAT_AI_CHAT_SIDE_PANEL_TAB_TRANSFER_BRIDGE_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_AI_CHAT_AI_CHAT_SIDE_PANEL_TAB_TRANSFER_BRIDGE_H_

#include <memory>

#include "base/memory/raw_ptr.h"

class BrowserWindowInterface;

namespace content {
class WebContents;
}  // namespace content

// Window-scoped controller (owned by `BrowserWindowFeatures`) that moves the
// live AI Chat `WebContents` between a browser tab and the side panel when the
// `kAIChatMoveFullPageToSidePanel` feature is enabled. It is intentionally
// small: it does not own the steady-state side panel contents (the
// `AIChatMovableSidePanelWebView` does) — only the transient hand-off used
// while a transfer is in-flight.
class AIChatSidePanelTabTransferBridge {
 public:
  explicit AIChatSidePanelTabTransferBridge(BrowserWindowInterface* browser);
  AIChatSidePanelTabTransferBridge(const AIChatSidePanelTabTransferBridge&) =
      delete;
  AIChatSidePanelTabTransferBridge& operator=(
      const AIChatSidePanelTabTransferBridge&) = delete;
  ~AIChatSidePanelTabTransferBridge();

  // Forward (tab -> side panel). Takes ownership of the live AI Chat
  // `web_contents` (already detached from the tab strip by the caller) and
  // (re)shows the AI Chat side panel entry so its factory adopts it.
  void TransferFullPageContentsToSidePanel(
      std::unique_ptr<content::WebContents> web_contents);

  // Whether a forward transfer is waiting to be adopted by the side panel view.
  bool HasPendingTransfer() const;

  // Hands the pending transfer contents to the caller (the side panel view
  // factory). Returns null when there is no pending transfer.
  std::unique_ptr<content::WebContents> TakePendingContents();

 private:
  // Clears any cached AI Chat side panel view (in both the window-scoped and
  // active tab-scoped registries) so the entry factory is guaranteed to run
  // again and adopt the pending contents.
  void ClearChatEntryCache();

  const raw_ptr<BrowserWindowInterface> browser_;

  // The live AI Chat contents in-flight from a tab to the side panel. Set by
  // `TransferFullPageContentsToSidePanel` and consumed by the side panel view
  // factory via `TakePendingContents`.
  std::unique_ptr<content::WebContents> pending_web_contents_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_AI_CHAT_AI_CHAT_SIDE_PANEL_TAB_TRANSFER_BRIDGE_H_
