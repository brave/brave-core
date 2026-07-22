// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_AI_CHAT_AI_CHAT_SIDE_PANEL_TAB_TRANSFER_BRIDGE_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_AI_CHAT_AI_CHAT_SIDE_PANEL_TAB_TRANSFER_BRIDGE_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "ui/views/view_observer.h"

class AIChatMovableSidePanelWebView;
class BrowserWindowInterface;

namespace content {
class WebContents;
}  // namespace content

namespace gfx {
class Rect;
}  // namespace gfx

namespace views {
class View;
}  // namespace views

// Window-scoped controller (owned by `BrowserWindowFeatures`) that moves the
// live AI Chat `WebContents` between a browser tab and the side panel when the
// `kAIChatMoveFullPageToSidePanel` feature is enabled. It is intentionally
// small: it does not own the steady-state side panel contents (the
// `AIChatMovableSidePanelWebView` does) — only the transient hand-off used
// while a transfer is in-flight.
class AIChatSidePanelTabTransferBridge : public views::ViewObserver {
 public:
  explicit AIChatSidePanelTabTransferBridge(BrowserWindowInterface* browser);
  AIChatSidePanelTabTransferBridge(const AIChatSidePanelTabTransferBridge&) =
      delete;
  AIChatSidePanelTabTransferBridge& operator=(
      const AIChatSidePanelTabTransferBridge&) = delete;
  ~AIChatSidePanelTabTransferBridge() override;

  // Forward (tab -> side panel). Takes ownership of the live AI Chat
  // `web_contents` (already detached from the tab strip by the caller) and
  // (re)shows the AI Chat side panel entry so its factory adopts it.
  // `starting_bounds_in_browser_coordinates` is the full-page contents' rect,
  // captured by the caller before detach; when non-empty the side panel content
  // animates in from it so the conversation appears to slide across rather than
  // flash. An empty rect falls back to a plain show.
  void TransferFullPageContentsToSidePanel(
      std::unique_ptr<content::WebContents> web_contents,
      const gfx::Rect& starting_bounds_in_browser_coordinates);

  // Whether a forward transfer is waiting to be adopted by the side panel view.
  bool HasPendingTransfer() const;

  // Hands the pending transfer contents to the caller (the side panel view
  // factory). Returns null when there is no pending transfer.
  std::unique_ptr<content::WebContents> TakePendingContents();

  // Tracks the live movable AI Chat side panel view (self-registered by
  // `AIChatMovableSidePanelWebView::CreateView`). Pass null to clear. The
  // bridge observes the view and clears the registration automatically when the
  // view is destroyed. The reverse move uses it to reach the panel's contents.
  void SetActiveChatView(AIChatMovableSidePanelWebView* view);
  AIChatMovableSidePanelWebView* active_chat_view() {
    return active_chat_view_;
  }

  // Reverse (side panel -> tab). Moves the live AI Chat `side_panel_contents`
  // out of the side panel and into a new full-page tab (preserving state), then
  // closes the panel. Returns true when it moved the contents; false (leaving
  // everything untouched) when `side_panel_contents` is not the live panel
  // conversation or is a tab-associated (contextual) conversation, in which
  // case the caller opens a fresh full-page tab instead.
  bool MoveSidePanelContentsToTab(content::WebContents* side_panel_contents);

  // views::ViewObserver:
  void OnViewIsDeleting(views::View* observed_view) override;

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

  // The live movable AI Chat side panel view for this window, or null when AI
  // Chat is not (movably) hosted in the side panel. Observed for destruction so
  // this back-pointer never dangles.
  raw_ptr<AIChatMovableSidePanelWebView> active_chat_view_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_AI_CHAT_AI_CHAT_SIDE_PANEL_TAB_TRANSFER_BRIDGE_H_
