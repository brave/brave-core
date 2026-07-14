// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/side_panel/ai_chat/ai_chat_side_panel_tab_transfer_controller.h"

#include <utility>

#include "base/check.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/side_panel/side_panel_enums.h"
#include "chrome/browser/ui/side_panel/side_panel_registry.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"

namespace {

SidePanelEntry::Key ChatEntryKey() {
  return SidePanelEntry::Key(SidePanelEntry::Id::kChatUI);
}

}  // namespace

AIChatSidePanelTabTransferController::AIChatSidePanelTabTransferController(
    BrowserWindowInterface* browser)
    : browser_(browser) {
  CHECK(browser_);
}

AIChatSidePanelTabTransferController::~AIChatSidePanelTabTransferController() =
    default;

void AIChatSidePanelTabTransferController::TransferFullPageContentsToSidePanel(
    std::unique_ptr<content::WebContents> web_contents) {
  CHECK(web_contents);

  SidePanelUI* side_panel_ui = SidePanelUI::From(browser_);
  if (!side_panel_ui) {
    // No side panel UI to host the conversation (e.g. a non-normal window).
    // Let `web_contents` be destroyed rather than leaking it.
    return;
  }

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

  side_panel_ui->Show(key);
}

bool AIChatSidePanelTabTransferController::HasPendingTransfer() const {
  return pending_web_contents_ != nullptr;
}

std::unique_ptr<content::WebContents>
AIChatSidePanelTabTransferController::TakePendingContents() {
  return std::move(pending_web_contents_);
}

void AIChatSidePanelTabTransferController::ClearChatEntryCache() {
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
