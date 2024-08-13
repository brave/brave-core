/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/split_view/split_view_location_bar.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "base/test/task_environment.h"
#include "components/omnibox/browser/location_bar_model_delegate.h"
#include "components/omnibox/browser/location_bar_model_impl.h"
#include "components/security_state/core/security_state.h"
#include "net/cert/cert_status_flags.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/layout_provider.h"

namespace {

class TestLocationBarModelDelegate : public LocationBarModelDelegate {
 public:
  void SetURL(const GURL& url) { url_ = url; }

  void SetShouldPreventElision(bool should_prevent_elision) {
    should_prevent_elision_ = should_prevent_elision;
  }

  void SetSecurityLevel(security_state::SecurityLevel level) {
    security_level_ = level;
  }

  void SetCertStatus(net::CertStatus cert_status) {
    cert_status_ = cert_status;
  }

  // LocationBarModelDelegate:
  std::u16string FormattedStringWithEquivalentMeaning(
      const GURL& url,
      const std::u16string& formatted_url) const override {
    return formatted_url + u"/TestSuffix";
  }

  bool GetURL(GURL* url) const override {
    *url = url_;
    return true;
  }

  bool ShouldPreventElision() override { return should_prevent_elision_; }

  security_state::SecurityLevel GetSecurityLevel() const override {
    return security_level_;
  }

  net::CertStatus GetCertStatus() const override { return cert_status_; }

  bool IsNewTabPage() const override { return false; }

  bool IsNewTabPageURL(const GURL& url) const override { return false; }

  bool IsHomePage(const GURL& url) const override { return false; }

 private:
  GURL url_;
  security_state::SecurityLevel security_level_;
  net::CertStatus cert_status_ = 0;
  bool should_prevent_elision_ = false;
};

class MockLocationBarModelDelegate
    : public testing::NiceMock<TestLocationBarModelDelegate> {
 public:
  ~MockLocationBarModelDelegate() override = default;

  // TestLocationBarModelDelegate:
  MOCK_METHOD(bool, GetURL, (GURL * url), (override, const));
  MOCK_METHOD(bool, IsNewTabPage, (), (override, const));
  MOCK_METHOD(bool, IsNewTabPageURL, (const GURL& url), (override, const));
};

}  // namespace

class SplitViewLocationBarUnitTest : public testing::Test {
 protected:
  SplitViewLocationBarUnitTest() = default;
  ~SplitViewLocationBarUnitTest() override = default;

  TestLocationBarModelDelegate& delegate() { return delegate_; }
  SplitViewLocationBar& location_bar() { return *location_bar_; }

  // testing::Test
  void SetUp() override {
    location_bar_ = std::make_unique<SplitViewLocationBar>(nullptr, nullptr);
    location_bar_->location_bar_model_ =
        std::make_unique<LocationBarModelImpl>(&delegate_, 1024);
  }

 private:
  base::test::TaskEnvironment task_environment_;
  views::LayoutProvider layout_provider;

  TestLocationBarModelDelegate delegate_;

  std::unique_ptr<SplitViewLocationBar> location_bar_ = nullptr;
};

TEST_F(SplitViewLocationBarUnitTest, GetURLForDisplay_HTTP) {
  // We always want http:// scheme to be shown
  GURL url("http://www.example.com");
  delegate().SetURL(url);
  EXPECT_EQ(u"example.com/TestSuffix", location_bar().GetURLForDisplay());
}

TEST_F(SplitViewLocationBarUnitTest, GetURLForDisplay_HTTPS) {
  GURL url("https://www.example.com");
  delegate().SetURL(url);
  EXPECT_EQ(u"example.com/TestSuffix", location_bar().GetURLForDisplay());

  delegate().SetShouldPreventElision(true);
  EXPECT_EQ(u"https://www.example.com/TestSuffix",
            location_bar().GetURLForDisplay());
}

TEST_F(SplitViewLocationBarUnitTest,
       UpdateURLAndIcon_CertErrorShouldShowHTTPSwithStrike) {
  GURL url("https://www.example.com");
  delegate().SetURL(url);
  delegate().SetCertStatus(net::CERT_STATUS_REVOKED);

  location_bar().UpdateURLAndIcon();

  EXPECT_EQ(u"example.com/TestSuffix", location_bar().url_->GetText());
  EXPECT_TRUE(location_bar().https_with_strike_->GetVisible());
  EXPECT_TRUE(location_bar().scheme_separator_->GetVisible());
}

TEST_F(SplitViewLocationBarUnitTest,
       UpdateIcon_InsecureContentsShouldShowWarningIcon) {
  delegate().SetSecurityLevel(security_state::DANGEROUS);
  location_bar().UpdateIcon();
  EXPECT_TRUE(location_bar().safety_icon_->GetVisible());

  delegate().SetSecurityLevel(security_state::SECURE);
  location_bar().UpdateIcon();
  EXPECT_FALSE(location_bar().safety_icon_->GetVisible());
}
