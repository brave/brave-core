/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/page_info/page_info_bubble_view.h"

#include "chrome/test/views/chrome_views_test_base.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/widget/widget.h"

class PageInfoBubbleViewTest : public ChromeViewsTestBase {
 protected:
  // ChromeViewsTestBase overrides:
  void SetUp() override {
    ChromeViewsTestBase::SetUp();
    CreateWidget();
  }

  void TearDown() override {
    if (widget_ && !widget_->IsClosed())
      widget_->Close();

    ChromeViewsTestBase::TearDown();
  }

  views::Widget* widget() const {
    return widget_;
  }

 private:
  void CreateWidget() {
    DCHECK(!widget_);

    widget_ = new views::Widget;
    views::Widget::InitParams params =
        CreateParams(views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
    widget_->Init(params);
  }

 private:
  views::Widget* widget_ = nullptr;
};

// Check InternalPageInfoBubbleView is used for brave url.
TEST_F(PageInfoBubbleViewTest, BraveURLTest) {
  PageInfoBubbleView::CreatePageInfoBubble(
      nullptr, gfx::Rect(), widget()->GetNativeWindow(),
      nullptr, nullptr, GURL("brave://sync/"), security_state::SecurityInfo());
  EXPECT_EQ(PageInfoBubbleViewBase::BUBBLE_INTERNAL_PAGE,
            PageInfoBubbleViewBase::GetShownBubbleType());
}
