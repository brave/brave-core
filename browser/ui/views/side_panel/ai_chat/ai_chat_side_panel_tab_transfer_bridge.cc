// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/side_panel/ai_chat/ai_chat_side_panel_tab_transfer_bridge.h"

#include <utility>

#include "base/check.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/side_panel/side_panel_enums.h"
#include "chrome/browser/ui/side_panel/side_panel_registry.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "ui/gfx/geometry/rect.h"

namespace {

SidePanelEntry::Key ChatEntryKey() {
  return SidePanelEntry::Key(SidePanelEntry::Id::kChatUI);
}

}  // namespace

AIChatSidePanelTabTransferBridge::AIChatSidePanelTabTransferBridge(
    BrowserWindowInterface* browser)
    : browser_(browser) {
  CHECK(browser_);
}

AIChatSidePanelTabTransferBridge::~AIChatSidePanelTabTransferBridge() = default;

void AIChatSidePanelTabTransferBridge::TransferFullPageContentsToSidePanel(
    std::unique_ptr<content::WebContents> web_contents,
    const gfx::Rect& starting_bounds_in_browser_coordinates) {
  CHECK(web_contents);
  // A transfer is consumed synchronously by the re-show below, so there should
  // never be one already in-flight.
  CHECK(!pending_web_contents_);

  // The bridge is only created for normal windows, which always have a side
  // panel UI.
  SidePanelUI* side_panel_ui = SidePanelUI::From(browser_);
  CHECK(side_panel_ui);

  pending_web_contents_ = std::move(web_contents);

  const SidePanelEntry::Key key = ChatEntryKey();

  // If AI Chat is already the visible side panel entry, close it first.
  // Otherwise `Show()` below early-returns on the already-showing entry and
  // never rebuilds the view to adopt the pending contents. Suppress animations
  // so the close (and the teardown of the previous view) happens synchronously.
  if (side_panel_ui->IsSidePanelEntryShowing(key)) {
    side_panel_ui->Close(SidePanelEntryHideReason::kSidePanelClosed,
                         /*suppress_animations=*/true);
  }

  // Drop any cached AI Chat view so the entry factory is guaranteed to run
  // again and adopt the pending contents (see
  // `AIChatMovableSidePanelWebView::CreateView`).
  ClearChatEntryCache();

  // Animate the conversation into the panel from where the full page currently
  // sits (flash-free). When the caller could not capture a starting rect (e.g.
  // the full-page contents view was unavailable), fall back to a plain show.
  if (starting_bounds_in_browser_coordinates.IsEmpty()) {
    side_panel_ui->Show(key);
  } else {
    side_panel_ui->ShowFrom(key, starting_bounds_in_browser_coordinates);
  }
}

bool AIChatSidePanelTabTransferBridge::HasPendingTransfer() const {
  return pending_web_contents_ != nullptr;
}

std::unique_ptr<content::WebContents>
AIChatSidePanelTabTransferBridge::TakePendingContents() {
  return std::move(pending_web_contents_);
}

void AIChatSidePanelTabTransferBridge::ClearChatEntryCache() {
  const SidePanelEntry::Key key = ChatEntryKey();

  // The AI Chat entry lives in the window-scoped registry when the side panel
  // is global, and in the active tab's registry when it is contextual. Clear
  // whichever holds it; the other simply has no such entry.
  if (SidePanelRegistry* global_registry = SidePanelRegistry::From(browser_)) {
    if (SidePanelEntry* entry = global_registry->GetEntryForKey(key)) {
      entry->ClearCachedView();
    }
  }

  if (tabs::TabInterface* active_tab = browser_->GetActiveTabInterface()) {
    if (SidePanelRegistry* tab_registry = SidePanelRegistry::From(active_tab)) {
      if (SidePanelEntry* entry = tab_registry->GetEntryForKey(key)) {
        entry->ClearCachedView();
      }
    }
  }
}
