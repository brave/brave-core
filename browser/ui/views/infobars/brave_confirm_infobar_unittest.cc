/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/infobars/brave_confirm_infobar.h"

#include "brave/components/infobars/core/brave_confirm_infobar_delegate.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

using BraveConfirmInfoBarTest = ChromeViewsTestBase;

namespace {

class TestInfoBarDelegate : public BraveConfirmInfoBarDelegate {
 public:
  TestInfoBarDelegate() = default;
  ~TestInfoBarDelegate() override = default;

  // BraveConfirmInfoBarDelegate:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override {
    return TEST_INFOBAR;
  }

  int GetButtons() const override { return BUTTON_NONE; }

  std::u16string GetMessageText() const override { return u""; }
};

}  // namespace

// Check close button is always the last children.
TEST_F(BraveConfirmInfoBarTest, CloseButtonOrderTest) {
  std::unique_ptr<views::Widget> widget =
      CreateTestWidget(views::Widget::InitParams::CLIENT_OWNS_WIDGET);
  auto infobar = std::make_unique<BraveConfirmInfoBar>(
      (std::make_unique<TestInfoBarDelegate>()));

  // Added to widget to get ViewHierarchyChanged().
  widget->SetContentsView(infobar.get());
  auto* close_button = infobar->close_button_for_testing();
  ASSERT_TRUE(close_button);
  EXPECT_EQ(close_button, infobar->children().back());
  infobar->DeprecatedLayoutImmediately();
}
