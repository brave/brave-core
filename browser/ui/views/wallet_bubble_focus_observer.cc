/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/wallet_bubble_focus_observer.h"

#include "chrome/browser/ui/views/bubble/webui_bubble_dialog_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/views/controls/webview/webview.h"

WalletBubbleFocusObserver::WalletBubbleFocusObserver(
    WebUIBubbleDialogView* web_ui_bubble_view,
    views::FocusManager* focus_manager)
    : web_ui_bubble_view_(web_ui_bubble_view), focus_manager_(focus_manager) {
  Subscribe();
}

WalletBubbleFocusObserver::~WalletBubbleFocusObserver() {
  Unsubscribe();
}

void WalletBubbleFocusObserver::Subscribe() {
  if (!web_ui_bubble_view_ || !focus_manager_ ||
      !web_ui_bubble_view_->web_view())
    return;
  web_ui_bubble_view_->web_view()->AddObserver(this);
  focus_manager_->AddFocusChangeListener(this);
}

void WalletBubbleFocusObserver::Unsubscribe() {
  if (web_ui_bubble_view_ && web_ui_bubble_view_->web_view())
    web_ui_bubble_view_->web_view()->RemoveObserver(this);
  if (focus_manager_)
    focus_manager_->RemoveFocusChangeListener(this);
}

bool WalletBubbleFocusObserver::IsBubbleLocked() const {
  return close_on_deactivate_.has_value();
}

void WalletBubbleFocusObserver::Lock(bool close_on_deactivate) {
  DCHECK(!IsBubbleLocked());
  // Save state to restore it when lock released.
  close_on_deactivate_ = close_on_deactivate;
  // Lock the bubble
  SetBubbleDeactivationState(false);
}

void WalletBubbleFocusObserver::ReleaseLock(bool close_on_deactivate) {
  DCHECK(IsBubbleLocked());
  SetBubbleDeactivationState(close_on_deactivate);
  close_on_deactivate_.reset();
}

// Update saved state to restore with new values.
void WalletBubbleFocusObserver::UpdateBubbleDeactivationState(bool state) {
  if (!IsBubbleLocked())
    return;

  close_on_deactivate_ = state;
}

void WalletBubbleFocusObserver::SetBubbleDeactivationState(
    bool close_on_deactivate) {
  if (!web_ui_bubble_view_)
    return;
  web_ui_bubble_view_->set_close_on_deactivate(close_on_deactivate);
}

bool WalletBubbleFocusObserver::GetCurrentBubbleDeactivationState() {
  return web_ui_bubble_view_->ShouldCloseOnDeactivate();
}

void WalletBubbleFocusObserver::CloseBubble() {
  if (!web_ui_bubble_view_)
    return;
  web_ui_bubble_view_->CloseUI();
}

// Called when the bubble webview captured focus.
// Set actual flag state back.
void WalletBubbleFocusObserver::OnViewFocused(views::View* observed_view) {
  if (!IsBubbleLocked())
    return;

  ReleaseLock(close_on_deactivate_.value());
}

// Set lock for closing until we get focus notification from the browser window.
void WalletBubbleFocusObserver::OnViewBlurred(views::View* observed_view) {
  Lock(GetCurrentBubbleDeactivationState());
}

// The focus is in the browser window now.
void WalletBubbleFocusObserver::OnWillChangeFocus(views::View* focused_before,
                                                  views::View* focused_now) {
  if (!IsBubbleLocked() || !focused_now)
    return;

  if (close_on_deactivate_.value()) {
    // Close the bubble since it has lost focus already.
    CloseBubble();
  }

  ReleaseLock(close_on_deactivate_.value());
}

void WalletBubbleFocusObserver::OnDidChangeFocus(views::View* focused_before,
                                                 views::View* focused_now) {}

// static
std::unique_ptr<WalletBubbleFocusObserver>
WalletBubbleFocusObserver::CreateForView(
    WebUIBubbleDialogView* web_ui_bubble_view,
    Browser* browser) {
  if (!browser || !web_ui_bubble_view)
    return nullptr;
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  if (!browser_view)
    return nullptr;

  if (!browser_view->GetFocusManager())
    return nullptr;

  return std::make_unique<WalletBubbleFocusObserver>(
      web_ui_bubble_view, browser_view->GetFocusManager());
}
