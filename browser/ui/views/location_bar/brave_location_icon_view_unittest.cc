/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/location_bar/brave_location_icon_view.h"

#include <memory>

#include "chrome/grit/chromium_strings.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "components/omnibox/browser/test_location_bar_model.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

class TestLocationIconViewDelegate : public LocationIconView::Delegate {
 public:
  explicit TestLocationIconViewDelegate(LocationBarModel* location_bar_model)
      : location_bar_model_(location_bar_model) {}

  // LocationIconView::Delegate overrides:
  content::WebContents* GetWebContents() override { return nullptr; }
  bool IsEditingOrEmpty() override { return false; }
  SkColor GetSecurityChipColor(
      security_state::SecurityLevel security_level) const override {
    return SK_ColorWHITE;
  }
  bool ShowPageInfoDialog() override { return false; }
  const LocationBarModel* GetLocationBarModel() const override {
    return location_bar_model_;
  }
  gfx::ImageSkia GetLocationIcon(
      IconFetchedCallback on_icon_fetched) const override {
    return gfx::ImageSkia();
  }
  SkColor GetLocationIconInkDropColor() const override {
    return SK_ColorWHITE;
  }

 private:
  LocationBarModel* location_bar_model_ = nullptr;
};

class BraveLocationIconViewTest : public ChromeViewsTestBase {
 protected:
  // ChromeViewsTestBase overrides:
  void SetUp() override {
    ChromeViewsTestBase::SetUp();

    gfx::FontList font_list;
    location_bar_model_ = std::make_unique<TestLocationBarModel>();
    delegate_ =
        std::make_unique<TestLocationIconViewDelegate>(location_bar_model());
    view_ = new BraveLocationIconView(font_list, delegate());
  }

  TestLocationBarModel* location_bar_model() {
    return location_bar_model_.get();
  }
  TestLocationIconViewDelegate* delegate() { return delegate_.get(); }
  LocationIconView* view() { return view_; }

 private:
  std::unique_ptr<TestLocationBarModel> location_bar_model_;
  std::unique_ptr<TestLocationIconViewDelegate> delegate_;
  LocationIconView* view_;
};

// Check LocationIconView shows proper icon and text for brave url.
TEST_F(BraveLocationIconViewTest, BraveURLTest) {
  location_bar_model()->set_url(GURL("brave://sync/"));
  location_bar_model()->set_input_in_progress(false);

  EXPECT_TRUE(view()->ShouldShowText());
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_SHORT_PRODUCT_NAME),
            view()->GetText());
}
