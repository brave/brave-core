/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/download/brave_download_item_model.h"

#include <stddef.h>
#include <stdint.h>

#include <vector>

#include "base/cxx17_backports.h"
#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/download/download_item_model.h"
#include "components/download/public/common/mock_download_item.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/text_utils.h"

using download::DownloadItem;
using safe_browsing::DownloadFileType;
using ::testing::_;
using ::testing::Mock;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::ReturnRefOfCopy;
using ::testing::SetArgPointee;

namespace {

// Default target path for a mock download item in DownloadItemModelTest.
const base::FilePath::CharType kDefaultTargetFilePath[] =
    FILE_PATH_LITERAL("/foo/bar/foo.bar");

const base::FilePath::CharType kDefaultDisplayFileName[] =
    FILE_PATH_LITERAL("foo.bar");

class BraveDownloadItemModelTest : public testing::Test {
 public:
  BraveDownloadItemModelTest() : model_(&item_), brave_model_(&model_) {}
  ~BraveDownloadItemModelTest() override {}

 protected:
  void SetupDownloadItemDefaults() {
    ON_CALL(item_, GetReceivedBytes()).WillByDefault(Return(1));
    ON_CALL(item_, GetTotalBytes()).WillByDefault(Return(2));
    ON_CALL(item_, TimeRemaining(_)).WillByDefault(Return(false));
    ON_CALL(item_, GetMimeType()).WillByDefault(Return("text/html"));
    ON_CALL(item_, AllDataSaved()).WillByDefault(Return(false));
    ON_CALL(item_, GetOpenWhenComplete()).WillByDefault(Return(false));
    ON_CALL(item_, GetFileExternallyRemoved()).WillByDefault(Return(false));
    ON_CALL(item_, GetState()).WillByDefault(Return(DownloadItem::IN_PROGRESS));
    ON_CALL(item_, GetURL()).WillByDefault(ReturnRefOfCopy(GURL("")));
    ON_CALL(item_, GetFileNameToReportUser())
        .WillByDefault(Return(base::FilePath(kDefaultDisplayFileName)));
    ON_CALL(item_, GetTargetFilePath())
        .WillByDefault(ReturnRefOfCopy(base::FilePath(kDefaultTargetFilePath)));
    ON_CALL(item_, GetTargetDisposition())
        .WillByDefault(Return(DownloadItem::TARGET_DISPOSITION_OVERWRITE));
    ON_CALL(item_, IsPaused()).WillByDefault(Return(false));
  }

  download::MockDownloadItem& item() { return item_; }
  BraveDownloadItemModel& model() { return brave_model_; }

 private:
  NiceMock<download::MockDownloadItem> item_;
  DownloadItemModel model_;
  BraveDownloadItemModel brave_model_;
};

}  // namespace

TEST_F(BraveDownloadItemModelTest, GetOriginUrlText) {
  // Test that we have the correct origin URL text.
  const struct TestCase {
    // The url.
    const char* url;
    // Expected text.
    const char* expected_text;
    // Expected is_secure.
    bool expected_is_secure;
  } kTestCases[] = {
      // Not secure.
      {"http://example.com/foo.bar", "http://example.com", false},
      // Secure.
      {"https://example.com:5678/foo.bar", "https://example.com:5678", true},
      // File, secure.
      {"file:///c:/foo/bar/foo.bar", "file:///", true},
      // about:blank, secure.
      {"about:blank", "about:blank", true},
      // about:srcdoc, secure.
      {"about:srcdoc", "about:srcdoc", true},
      // Other about: URLs, not secure.
      {"about:about", "about:about", true},
      // invalid, not secure.
      {"foo.bar.baz", "", false},
      // empty, not secure.
      {"", "", false},
  };

  SetupDownloadItemDefaults();
  for (unsigned i = 0; i < base::size(kTestCases); ++i) {
    const TestCase& test_case = kTestCases[i];
    EXPECT_CALL(item(), GetURL())
        .WillRepeatedly(ReturnRefOfCopy(GURL(test_case.url)));
    bool is_secure = false;
    EXPECT_STREQ(
        test_case.expected_text,
        base::UTF16ToUTF8(model().GetOriginURLText(&is_secure)).c_str());
    EXPECT_EQ(is_secure, test_case.expected_is_secure) << test_case.url;
    Mock::VerifyAndClearExpectations(&item());
  }
}

TEST_F(BraveDownloadItemModelTest, GetTooltipText) {
  // Test that we have the correct tooltip text that includes origin URL.
  const struct TestCase {
    // The url.
    const char* url;
    // Expected tooltip text.
    const char* expected_tooltip;
  } kTestCases[] = {
    {"http://example.com/foo.bar", "foo.bar\nnot secure http://example.com"},
    {"https://example.com:5678/foo.bar", "foo.bar\nhttps://example.com:5678"},
  };

  SetupDownloadItemDefaults();
  for (unsigned i = 0; i < base::size(kTestCases); ++i) {
    const TestCase& test_case = kTestCases[i];
    EXPECT_CALL(item(), GetURL())
        .WillRepeatedly(ReturnRefOfCopy(GURL(test_case.url)));
    EXPECT_EQ(base::ToLowerASCII(base::UTF16ToUTF8(model().GetTooltipText())),
              test_case.expected_tooltip);
    Mock::VerifyAndClearExpectations(&item());
  }
}
