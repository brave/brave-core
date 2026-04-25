// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/toolbar/wallet_button.h"

#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "ui/views/test/button_test_api.h"

namespace {
ui::MouseEvent GetDummyEvent() {
  return ui::MouseEvent(ui::EventType::kMousePressed, gfx::PointF(),
                        gfx::PointF(), base::TimeTicks::Now(), 0, 0);
}
}  // namespace

namespace brave_wallet {

class WalletButtonButtonBrowserTest : public InProcessBrowserTest {
 public:
  BrowserView* browser_view() {
    return BrowserView::GetBrowserViewForBrowser(browser());
  }

  WalletButton* wallet_button() {
    return static_cast<BraveBrowserView*>(browser_view())->GetWalletButton();
  }
};

// Regression test: SetBraveButtonFlexBehavior previously used the two-argument
// FlexSpecification constructor which applies kPreferredSnapToZero to both the
// width and height axes. When the wallet badge image is shown its preferred
// height exceeds the toolbar's cross-axis budget, causing height to snap to
// zero (width=44, height=0) and the button to become invisible. The fix uses
// the orientation-aware constructor so only the width axis can snap to zero.
IN_PROC_BROWSER_TEST_F(WalletButtonButtonBrowserTest,
                       SizeIsNotEmptyWithAndWithoutBadge) {
  // counter = 0: no badge, button should be visible with normal size.
  ASSERT_TRUE(wallet_button()->GetVisible());
  EXPECT_FALSE(wallet_button()->size().IsEmpty())
      << "Wallet button has empty size with no badge";

  // counter > 0: badge image is taller than the base icon, which previously
  // caused the flex rule to snap height to zero.
  wallet_button()->OnNotificationUpdate(false, 99);
  RunScheduledLayouts();
  EXPECT_FALSE(wallet_button()->size().IsEmpty())
      << "Wallet button has empty size when badge is shown";

  wallet_button()->OnNotificationUpdate(false, 0);
  RunScheduledLayouts();
  EXPECT_FALSE(wallet_button()->size().IsEmpty())
      << "Wallet button has empty size with no badge";
}

IN_PROC_BROWSER_TEST_F(WalletButtonButtonBrowserTest,
                       ButtonClickCreatesBubble) {
  ASSERT_FALSE(wallet_button()->IsShowingBubble());
  views::test::ButtonTestApi(wallet_button()).NotifyClick(GetDummyEvent());
  ASSERT_TRUE(wallet_button()->IsShowingBubble());

  wallet_button()->CloseWalletBubble();
  ASSERT_TRUE(wallet_button()->IsBubbleClosedForTesting());
}

class WalletButtonBrowserUITest : public DialogBrowserTest {
 public:
  // DialogBrowserTest:
  void ShowUi(const std::string& name) override {
    auto* wallet_button = static_cast<BraveBrowserView*>(
                              BrowserView::GetBrowserViewForBrowser(browser()))
                              ->GetWalletButton();
    views::test::ButtonTestApi(wallet_button).NotifyClick(GetDummyEvent());
  }
};

// Invokes a wallet panel bubble.
IN_PROC_BROWSER_TEST_F(WalletButtonBrowserUITest, InvokeUi_default) {
  ShowAndVerifyUi();
}

}  // namespace brave_wallet
