// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include <memory>

#include "brave/components/brave_today/browser/urls.h"

#include "brave/components/l10n/browser/locale_helper_mock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

bool EndsWithJSONExtension(GURL url) {
  std::string ending = ".json";
  return ((url.spec().length() >= ending.length()) &&
          (url.spec().compare(url.spec().length() - ending.length(),
                              ending.length(), ending)));
}

}  // namespace

namespace brave_news {

class BraveNewsURLsTest : public testing::Test {
 public:
  void SetMockLocale(const std::string& locale) {
    locale_helper_mock_ =
        std::make_unique<NiceMock<brave_l10n::LocaleHelperMock>>();
    brave_l10n::LocaleHelper::GetInstance()->set_for_testing(
        locale_helper_mock_.get());
    ON_CALL(*locale_helper_mock_, GetLocale()).WillByDefault(Return(locale));
  }

 protected:
  std::unique_ptr<brave_l10n::LocaleHelperMock> locale_helper_mock_;
};

TEST_F(BraveNewsURLsTest, CorrectFormatNonDefaultLang) {
  SetMockLocale("ja_JP");
  GURL feed_url_ja = GetFeedUrl();
  // Check we avoid the issue at
  // https://github.com/brave/brave-browser/issues/20114
  EXPECT_TRUE(EndsWithJSONExtension(feed_url_ja));
  // Check is customized (different to default)
  SetMockLocale("en_US");
  GURL feed_url_default = GetFeedUrl();
  bool ja_is_custom_feed = (feed_url_ja.spec() != feed_url_default);
  EXPECT_TRUE(ja_is_custom_feed);
}

TEST_F(BraveNewsURLsTest, CorrectFormatDefaultLang) {
  SetMockLocale("en_US");
  GURL feed_url_default = GetFeedUrl();
  EXPECT_TRUE(EndsWithJSONExtension(feed_url_default));
}

}  // namespace brave_news
