// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/toolbar/wallet_button.h"

#include "base/feature_list.h"
#include "base/run_loop.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/components/brave_wallet/common/features.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "ui/base/ui_base_types.h"
#include "ui/views/test/button_test_api.h"
#include "ui/views/widget/widget.h"

namespace {
ui::MouseEvent GetDummyEvent() {
  return ui::MouseEvent(ui::EventType::kMousePressed, gfx::PointF(),
                        gfx::PointF(), base::TimeTicks::Now(), 0, 0);
}
}  // namespace

namespace brave_wallet {

class WalletButtonButtonBrowserTest : public InProcessBrowserTest {
 public:
  // InProcessBrowserTest:
  void SetUp() override {
    scoped_feature_list_.InitAndEnableFeature(
        features::kNativeBraveWalletFeature);
    InProcessBrowserTest::SetUp();
  }

  BrowserView* browser_view() {
    return BrowserView::GetBrowserViewForBrowser(browser());
  }

  WalletButton* wallet_button() {
    return static_cast<BraveBrowserView*>(browser_view())->GetWalletButton();
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(WalletButtonButtonBrowserTest,
                       ButtonClickCreatesBubble) {
  ASSERT_FALSE(wallet_button()->IsShowingBubble());
  views::test::ButtonTestApi(wallet_button()).NotifyClick(GetDummyEvent());
  ASSERT_TRUE(wallet_button()->IsShowingBubble());

  wallet_button()->CloseWalletBubble();
  ASSERT_TRUE(wallet_button()->IsBubbleClosedForTesting());
}

#if BUILDFLAG(IS_MAC)
// Flaky on Mac CI.
#define MAYBE_ZOrderLevelShouldBeSecuritySurface \
  DISABLED_ZOrderLevelShouldBeSecuritySurface
#else
#define MAYBE_ZOrderLevelShouldBeSecuritySurface \
  ZOrderLevelShouldBeSecuritySurface
#endif

IN_PROC_BROWSER_TEST_F(WalletButtonButtonBrowserTest,
                       MAYBE_ZOrderLevelShouldBeSecuritySurface) {
  // Create widget
  ASSERT_FALSE(wallet_button()->IsShowingBubble());
  views::test::ButtonTestApi(wallet_button()).NotifyClick(GetDummyEvent());

  // Wait until the bubble is shown and widget is created.
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return wallet_button()->IsShowingBubble(); }));

  // Wait until the widget is created and active.
  auto* widget = wallet_button()->GetBubbleWidgetForTesting();
  ASSERT_TRUE(widget);
  ASSERT_TRUE(base::test::RunUntil([&]() { return widget->IsActive(); }));

  // When the bubble is shown, the widget should have the z-order level of
  // kSecuritySurface.
  EXPECT_EQ(widget->GetZOrderLevel(), ui::ZOrderLevel::kSecuritySurface);

  // Also parent widget should have the same z-order level.
  auto* parent_widget = widget->parent();
  ASSERT_TRUE(parent_widget);
  EXPECT_EQ(parent_widget->GetZOrderLevel(), ui::ZOrderLevel::kSecuritySurface);

  auto widget_weak_ptr = widget->GetWeakPtr();
  ASSERT_TRUE(widget_weak_ptr);
  auto* parent_widget_weak_ptr = widget_weak_ptr->parent();

  wallet_button()->CloseWalletBubble();

  ASSERT_TRUE(base::test::RunUntil([&]() { return !widget_weak_ptr; }));
  EXPECT_NE(parent_widget_weak_ptr->GetZOrderLevel(),
            ui::ZOrderLevel::kSecuritySurface);
}

class WalletButtonBrowserUITest : public DialogBrowserTest {
 public:
  // DialogBrowserTest:
  void SetUp() override {
    feature_list_.InitAndEnableFeature(features::kNativeBraveWalletFeature);
    DialogBrowserTest::SetUp();
  }
  void ShowUi(const std::string& name) override {
    auto* wallet_button = static_cast<BraveBrowserView*>(
                              BrowserView::GetBrowserViewForBrowser(browser()))
                              ->GetWalletButton();
    views::test::ButtonTestApi(wallet_button).NotifyClick(GetDummyEvent());
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// Invokes a wallet panel bubble.
IN_PROC_BROWSER_TEST_F(WalletButtonBrowserUITest, InvokeUi_default) {
  ShowAndVerifyUi();
}

}  // namespace brave_wallet
