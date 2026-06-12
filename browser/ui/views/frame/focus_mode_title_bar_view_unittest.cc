/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/focus_mode_title_bar_view.h"

#include <memory>
#include <string>
#include <utility>

#include "base/callback_list.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tab_ui_helper.h"
#include "chrome/browser/ui/tabs/public/tab_features.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "components/tabs/public/mock_tab_interface.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/unowned_user_data/unowned_user_data_host.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/controls/label.h"
#include "ui/views/test/views_test_utils.h"
#include "url/gurl.h"

namespace {

class FakeTab : public tabs::MockTabInterface {
 public:
  explicit FakeTab(Profile* profile)
      : tab_features_(std::make_unique<tabs::TabFeatures>()),
        contents_(content::WebContentsTester::CreateTestWebContents(profile,
                                                                    nullptr)) {
    using ::testing::Return;
    using ::testing::ReturnRef;

    ON_CALL(*this, GetContents()).WillByDefault(Return(contents_.get()));
    ON_CALL(*this, GetTabFeatures()).WillByDefault(Return(tab_features_.get()));
    ON_CALL(*this, GetUnownedUserDataHost()).WillByDefault(ReturnRef(host_));
    ON_CALL(*this, RegisterWillDetach(::testing::_))
        .WillByDefault([this](tabs::TabInterface::WillDetach callback) {
          return will_detach_callbacks_.Add(std::move(callback));
        });

    tab_features_->SetTabUIHelperForTesting(
        std::make_unique<TabUIHelper>(*this));
  }

  ~FakeTab() override {
    will_detach_callbacks_.Notify(this,
                                  tabs::TabInterface::DetachReason::kDelete);
  }

  void NavigateAndCommit(const GURL& url) {
    content::WebContentsTester::For(contents_.get())->NavigateAndCommit(url);
  }

 private:
  ui::UnownedUserDataHost host_;
  std::unique_ptr<tabs::TabFeatures> tab_features_;
  std::unique_ptr<content::WebContents> contents_;
  base::RepeatingCallbackList<void(tabs::TabInterface*,
                                   tabs::TabInterface::DetachReason)>
      will_detach_callbacks_;
};

class FocusModeTitleBarViewTest : public ChromeViewsTestBase {
 public:
  void SetUp() override {
    ChromeViewsTestBase::SetUp();
    view_ = std::make_unique<FocusModeTitleBarView>();
  }

  void TearDown() override {
    view_.reset();
    ChromeViewsTestBase::TearDown();
  }

 protected:
  std::unique_ptr<FakeTab> AddTab(const GURL& url) {
    auto tab = std::make_unique<FakeTab>(&profile_);
    tab->NavigateAndCommit(url);
    return tab;
  }

  std::u16string_view GetDomainText() {
    return view_->domain_label_for_testing()->GetText();
  }

  std::u16string_view AddTabAndGetDomainText(const GURL& gurl) {
    auto tab = AddTab(gurl);
    view_->SetTab(tab.get());
    return GetDomainText();
  }

  std::unique_ptr<FocusModeTitleBarView> view_;
  TestingProfile profile_;
  content::RenderViewHostTestEnabler render_view_host_test_enabler_;
};

TEST_F(FocusModeTitleBarViewTest, UpdatesOnNavigation) {
  auto tab = AddTab(GURL("https://www.example.com/some/path?q=1"));
  view_->SetTab(tab.get());
  EXPECT_EQ(GetDomainText(), u"example.com");

  tab->NavigateAndCommit(GURL("https://docs.example.com/title1.html"));
  EXPECT_EQ(GetDomainText(), u"docs.example.com");

  tab->NavigateAndCommit(GURL("http://insecure.example.com/"));
  EXPECT_EQ(GetDomainText(), u"http://insecure.example.com");

  tab->NavigateAndCommit(GURL("chrome://abc"));
  EXPECT_EQ(GetDomainText(), u"brave://abc");

  tab->NavigateAndCommit(GURL("chrome-untrusted://print"));
  EXPECT_EQ(GetDomainText(), u"chrome-untrusted://print");
}

TEST_F(FocusModeTitleBarViewTest, ClearsWhenTabDetaches) {
  auto tab = AddTab(GURL("https://www.example.com/title1.html"));
  view_->SetTab(tab.get());
  ASSERT_EQ(GetDomainText(), u"example.com");

  tab.reset();
  EXPECT_TRUE(GetDomainText().empty());
  EXPECT_FALSE(view_->IsFaviconVisibleForTesting());
}

TEST_F(FocusModeTitleBarViewTest, ElidesDomainFromHead) {
  auto tab = AddTab(GURL("https://very-long-spoofing.example.com/"));
  view_->SetTab(tab.get());
  ASSERT_EQ(GetDomainText(), u"very-long-spoofing.example.com");

  view_->SetSize(gfx::Size(60, 20));
  views::test::RunScheduledLayout(view_.get());

  std::u16string_view displayed =
      view_->domain_label_for_testing()->GetDisplayTextForTesting();
  EXPECT_TRUE(displayed.starts_with(u"\u2026"));
  EXPECT_TRUE(displayed.ends_with(u".com"));
}

TEST_F(FocusModeTitleBarViewTest, UpdatesOnTabSwitch) {
  auto first = AddTab(GURL("https://first.test/title1.html"));
  view_->SetTab(first.get());
  EXPECT_EQ(GetDomainText(), u"first.test");

  auto second = AddTab(GURL("https://second.test/title1.html"));
  view_->SetTab(second.get());
  EXPECT_EQ(GetDomainText(), u"second.test");

  view_->SetTab(nullptr);
  EXPECT_TRUE(GetDomainText().empty());
  EXPECT_FALSE(view_->IsFaviconVisibleForTesting());
}

}  // namespace
