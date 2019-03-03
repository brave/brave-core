/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_status_bubble_views.h"

#include "chrome/test/views/chrome_views_test_base.h"
#include "ui/views/widget/widget.h"

class BraveStatusBubbleViewsTest : public ChromeViewsTestBase {
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

  views::Widget* widget_ = nullptr;
};

TEST_F(BraveStatusBubbleViewsTest, SetURLTest) {
  BraveStatusBubbleViews bubble(widget()->GetContentsView());
  bubble.SetURL(GURL("chrome://settings/"));
  EXPECT_EQ(GURL("brave://settings/"), bubble.url_);

  const GURL brave_url("https://www.brave.com/");
  bubble.SetURL(brave_url);
  EXPECT_EQ(brave_url, bubble.url_);
}
