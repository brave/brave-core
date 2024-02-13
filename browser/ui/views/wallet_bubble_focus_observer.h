/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_WALLET_BUBBLE_FOCUS_OBSERVER_H_
#define BRAVE_BROWSER_UI_VIEWS_WALLET_BUBBLE_FOCUS_OBSERVER_H_

#include <memory>
#include <optional>

#include "base/gtest_prod_util.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/view_observer.h"

namespace views {
class View;
}  // namespace views

class Browser;
class WebUIBubbleDialogView;

class WalletBubbleFocusObserver : public views::ViewObserver,
                                  public views::FocusChangeListener {
 public:
  static std::unique_ptr<WalletBubbleFocusObserver> CreateForView(
      WebUIBubbleDialogView* web_ui_bubble_view,
      Browser* browser);

  explicit WalletBubbleFocusObserver(WebUIBubbleDialogView* web_ui_bubble_view,
                                     views::FocusManager* focus_manager);

  void UpdateBubbleDeactivationState(bool state);
  ~WalletBubbleFocusObserver() override;

  WalletBubbleFocusObserver(const WalletBubbleFocusObserver&) = delete;
  WalletBubbleFocusObserver& operator=(const WalletBubbleFocusObserver&) =
      delete;

 protected:
  FRIEND_TEST_ALL_PREFIXES(WalletBubbleFocusObserverUnitTest,
                           FocusOutFromWindowAndBackToPanel);
  FRIEND_TEST_ALL_PREFIXES(WalletBubbleFocusObserverUnitTest,
                           ClosePanelWhenFocusOutAndBackToBrowserWindow);
  FRIEND_TEST_ALL_PREFIXES(WalletBubbleFocusObserverUnitTest,
                           UpdatePanelStateWhenUnfocused);

  void ReleaseLock(bool close_on_deactivate);
  void Lock(bool close_on_deactivate);
  bool IsBubbleLocked() const;

  // Methods are virtual for testing purposes.
  virtual void Subscribe();
  virtual void Unsubscribe();
  virtual void SetBubbleDeactivationState(bool close_on_deactivate);
  virtual bool GetCurrentBubbleDeactivationState();
  virtual void CloseBubble();

  std::optional<bool> close_on_deactivate() { return close_on_deactivate_; }
  // views::ViewObserver
  // Listening Bubble's WebView focus events
  void OnViewFocused(views::View* observed_view) override;
  void OnViewBlurred(views::View* observed_view) override;

  // views::FocusChangeListener
  // Listening Bubble's anchor browser focus events
  void OnWillChangeFocus(views::View* focused_before,
                         views::View* focused_now) override;
  void OnDidChangeFocus(views::View* focused_before,
                        views::View* focused_now) override;

 private:
  raw_ptr<WebUIBubbleDialogView> const web_ui_bubble_view_;
  raw_ptr<views::FocusManager> const focus_manager_;
  std::optional<bool> close_on_deactivate_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_WALLET_BUBBLE_FOCUS_OBSERVER_H_
