/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/wallet_bubble_focus_observer.h"

#include "chrome/browser/ui/views/bubble/webui_bubble_dialog_view.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/view_observer.h"

class WebUIBubbleDialogView;

class TestWalletBubbleFocusObserver : public WalletBubbleFocusObserver {
 public:
  TestWalletBubbleFocusObserver(WebUIBubbleDialogView* web_ui_bubble_view,
                                views::FocusManager* focus_manager)
      : WalletBubbleFocusObserver(web_ui_bubble_view, focus_manager) {
    Subscribe();
  }

  ~TestWalletBubbleFocusObserver() override = default;

  void Subscribe() override { subscribed_ = true; }
  void Unsubscribe() override { subscribed_ = false; }
  void SetBubbleDeactivationState(bool close_on_deactivate) override {
    bubble_deactivation_state_ = close_on_deactivate;
  }
  bool GetCurrentBubbleDeactivationState() override {
    return bubble_deactivation_state_;
  }
  void CloseBubble() override { close_buble_called_ = true; }

  bool subscribed() { return subscribed_; }
  bool locked() { return locked_; }
  bool saved_deactivation_flag() { return saved_deactivation_flag_; }
  bool bubble_deactivation_state() { return bubble_deactivation_state_; }
  bool close_buble_called() { return close_buble_called_; }

 private:
  bool bubble_deactivation_state_ = true;
  bool close_buble_called_ = false;
  bool subscribed_ = false;
  bool locked_ = false;
  bool saved_deactivation_flag_ = false;
};

TEST(WalletBubbleFocusObserverUnitTest, FocusOutFromWindowAndBackToPanel) {
  TestWalletBubbleFocusObserver observer(nullptr, nullptr);
  ASSERT_TRUE(observer.subscribed());
  ASSERT_FALSE(observer.IsBubbleLocked());
  EXPECT_EQ(observer.close_on_deactivate(), absl::nullopt);
  ASSERT_TRUE(observer.bubble_deactivation_state());
  // Focus out of view.
  observer.OnViewBlurred(nullptr);
  ASSERT_FALSE(observer.bubble_deactivation_state());
  ASSERT_TRUE(observer.close_on_deactivate());

  // Focus returned back to bubble view.
  observer.OnViewFocused(nullptr);
  ASSERT_TRUE(observer.bubble_deactivation_state());
  EXPECT_EQ(observer.close_on_deactivate(), absl::nullopt);
}

TEST(WalletBubbleFocusObserverUnitTest,
     ClosePanelWhenFocusOutAndBackToBrowserWindow) {
  TestWalletBubbleFocusObserver observer(nullptr, nullptr);
  ASSERT_TRUE(observer.subscribed());
  ASSERT_FALSE(observer.IsBubbleLocked());
  EXPECT_EQ(observer.close_on_deactivate(), absl::nullopt);
  ASSERT_TRUE(observer.bubble_deactivation_state());
  ASSERT_FALSE(observer.close_buble_called());
  // Focus out of view.
  observer.OnViewBlurred(nullptr);
  ASSERT_FALSE(observer.bubble_deactivation_state());
  ASSERT_TRUE(observer.close_on_deactivate());
  ASSERT_FALSE(observer.close_buble_called());
  // Focus returned back to bubble view.
  views::View view;
  observer.OnWillChangeFocus(nullptr, &view);
  ASSERT_TRUE(observer.bubble_deactivation_state());
  EXPECT_EQ(observer.close_on_deactivate(), absl::nullopt);
  ASSERT_TRUE(observer.close_buble_called());
}

TEST(WalletBubbleFocusObserverUnitTest, UpdatePanelStateWhenUnfocused) {
  TestWalletBubbleFocusObserver observer(nullptr, nullptr);
  ASSERT_TRUE(observer.subscribed());
  ASSERT_FALSE(observer.IsBubbleLocked());
  EXPECT_EQ(observer.close_on_deactivate(), absl::nullopt);
  ASSERT_TRUE(observer.bubble_deactivation_state());
  ASSERT_FALSE(observer.close_buble_called());
  // Focus out of view.
  observer.OnViewBlurred(nullptr);
  ASSERT_FALSE(observer.bubble_deactivation_state());
  ASSERT_TRUE(observer.close_on_deactivate());
  ASSERT_FALSE(observer.close_buble_called());

  // Some API call blocked the bubble form closing while
  // user was in another window.
  observer.UpdateBubbleDeactivationState(false);

  // Focus returned back to bubble view.
  views::View view;
  observer.OnWillChangeFocus(nullptr, &view);
  ASSERT_FALSE(observer.bubble_deactivation_state());
  EXPECT_EQ(observer.close_on_deactivate(), absl::nullopt);
  // We should not close the bubble if user interacts with the browser window
  ASSERT_FALSE(observer.close_buble_called());

  // Focus returned back to bubble view.
  observer.OnViewFocused(nullptr);
  ASSERT_FALSE(observer.bubble_deactivation_state());
  EXPECT_EQ(observer.close_on_deactivate(), absl::nullopt);
}
