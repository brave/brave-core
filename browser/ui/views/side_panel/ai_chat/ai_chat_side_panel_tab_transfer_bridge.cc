// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/side_panel/ai_chat/ai_chat_side_panel_tab_transfer_bridge.h"

#include <utility>

#include "base/check.h"
#include "brave/browser/ui/views/side_panel/ai_chat/ai_chat_movable_side_panel_web_view.h"
#include "brave/components/ai_chat/core/common/ai_chat_urls.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/navigator/browser_navigator.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "chrome/browser/ui/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/side_panel/side_panel_enums.h"
#include "chrome/browser/ui/side_panel/side_panel_registry.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "chrome/browser/ui/webui/webui_embedding_context.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "extensions/buildflags/buildflags.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/view.h"

#if BUILDFLAG(ENABLE_EXTENSIONS_CORE)
#include "extensions/browser/view_type_utils.h"
#endif

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

AIChatSidePanelTabTransferBridge::~AIChatSidePanelTabTransferBridge() {
  // Stop observing the active view (if any) before this bridge goes away.
  SetActiveChatView(nullptr);
}

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

void AIChatSidePanelTabTransferBridge::SetActiveChatView(
    AIChatMovableSidePanelWebView* view) {
  if (active_chat_view_ == view) {
    return;
  }
  if (active_chat_view_) {
    active_chat_view_->RemoveObserver(this);
  }
  active_chat_view_ = view;
  if (active_chat_view_) {
    active_chat_view_->AddObserver(this);
  }
}

bool AIChatSidePanelTabTransferBridge::MoveSidePanelContentsToTab(
    content::WebContents* side_panel_contents) {
  // Only the live conversation currently hosted in the side panel can be moved.
  // If there is no active movable chat view, or the caller is some other
  // contents, there is nothing to move.
  if (!active_chat_view_ ||
      active_chat_view_->web_contents() != side_panel_contents) {
    return false;
  }

  // A tab-associated (contextual) conversation follows the active tab and
  // carries `/tab` semantics that don't belong in a standalone full-page tab;
  // leave it to the caller's fresh-tab path.
  if (ai_chat::ConversationUUIDFromURL(
          side_panel_contents->GetLastCommittedURL()) == "tab") {
    return false;
  }

  // The bridge is only created for normal windows, which always have a side
  // panel UI.
  SidePanelUI* side_panel_ui = SidePanelUI::From(browser_);
  CHECK(side_panel_ui);

  // Take the live contents out of the view (not destroyed, not reloaded).
  std::unique_ptr<content::WebContents> web_contents =
      active_chat_view_->ReleaseWebContents();
  CHECK(web_contents);

  // Restore the associations the contents needs as a tab, mirroring
  // `ContextualTasksSidePanelCoordinator::DetachWebContentsForTask`.
#if BUILDFLAG(ENABLE_EXTENSIONS_CORE)
  // Back to a tab-hosted view type (the movable view set `kComponent`).
  extensions::SetViewType(web_contents.get(),
                          extensions::mojom::ViewType::kTabContents);
#endif
  // The movable view is no longer a valid delegate once the contents leaves it.
  web_contents->SetDelegate(nullptr);
  // Drop the panel's browser-window association. Tab insertion re-establishes
  // the tab (and its window) association; the webui embedding context CHECKs
  // that the browser and tab associations are never set at the same time.
  webui::SetBrowserWindowInterface(web_contents.get(), nullptr);

  // Insert the live contents into a new foreground tab. The
  // `BrowserWindowInterface*` + `unique_ptr<WebContents>` ctor keeps this
  // `Browser*`-free; `Navigate` re-runs the idempotent `AttachTabHelpers`.
  NavigateParams params(browser_, std::move(web_contents));
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params.transition = ui::PAGE_TRANSITION_LINK;
  Navigate(&params);

  // The conversation now lives in a tab; close the (now-empty) panel. Its view
  // is torn down on close, but its owned contents was already released.
  side_panel_ui->Close();
  return true;
}

void AIChatSidePanelTabTransferBridge::OnViewIsDeleting(
    views::View* observed_view) {
  if (observed_view == active_chat_view_) {
    SetActiveChatView(nullptr);
  }
}
