/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/safe_browsing/brave_download_protection_delegate_desktop.h"

#include <memory>

#include "base/files/file_path.h"
#include "brave/components/safebrowsing/constants.h"
#include "chrome/test/base/testing_profile.h"
#include "components/download/public/common/mock_download_item.h"
#include "components/safe_browsing/core/common/safe_browsing_prefs.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/browser/download_item_utils.h"
#include "content/public/browser/file_system_access_write_item.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace safe_browsing {

namespace {

using ::testing::NiceMock;
using ::testing::ReturnRef;

}  // namespace

class BraveDownloadProtectionDelegateDesktopTest : public testing::Test {
 public:
  void SetUp() override {
    profile_->GetTestingPrefService()->registry()->RegisterBooleanPref(
        kBraveSafeBrowsingDownloadProtectionEnabled, true);
    // Parent delegate gates on upstream Safe Browsing being enabled; turn it on
    // so the only variable under test is Brave's download-protection pref.
    profile_->GetPrefs()->SetBoolean(prefs::kSafeBrowsingEnabled, true);
  }

  void SetBravePref(bool enabled) {
    profile_->GetPrefs()->SetBoolean(
        kBraveSafeBrowsingDownloadProtectionEnabled, enabled);
  }

  std::unique_ptr<download::MockDownloadItem> CreateDownloadItem(
      const base::FilePath& target_path) {
    auto item = std::make_unique<NiceMock<download::MockDownloadItem>>();
    content::DownloadItemUtils::AttachInfoForTesting(item.get(), profile_.get(),
                                                     nullptr);
    ON_CALL(*item, GetURL()).WillByDefault(ReturnRef(url_));
    ON_CALL(*item, GetTargetFilePath()).WillByDefault(ReturnRef(target_path));
    return item;
  }

  std::unique_ptr<content::FileSystemAccessWriteItem> CreateWriteItem(
      const base::FilePath& target_path) {
    auto item = std::make_unique<content::FileSystemAccessWriteItem>();
    item->browser_context = profile_.get();
    item->target_file_path = target_path;
    return item;
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_ = std::make_unique<TestingProfile>();
  BraveDownloadProtectionDelegateDesktop delegate_;
  const GURL url_{"https://example.com/file.exe"};
};

// With Brave's download protection off, every gated method returns false even
// though upstream Safe Browsing is enabled.
TEST_F(BraveDownloadProtectionDelegateDesktopTest, PrefOffShortCircuits) {
  SetBravePref(false);

  const base::FilePath target(FILE_PATH_LITERAL("file.exe"));
  auto item = CreateDownloadItem(target);

  EXPECT_FALSE(delegate_.ShouldCheckDownloadUrl(item.get()));
  EXPECT_FALSE(delegate_.MayCheckClientDownload(item.get()));

  auto write_item = CreateWriteItem(target);
  EXPECT_FALSE(delegate_.MayCheckFileSystemAccessWrite(write_item.get()));
}

// With Brave's download protection on, the override defers to the upstream
// desktop delegate, which permits the check when Safe Browsing is enabled.
TEST_F(BraveDownloadProtectionDelegateDesktopTest, PrefOnDelegatesToParent) {
  SetBravePref(true);

  const base::FilePath target(FILE_PATH_LITERAL("file.exe"));
  auto item = CreateDownloadItem(target);

  EXPECT_TRUE(delegate_.ShouldCheckDownloadUrl(item.get()));
  EXPECT_TRUE(delegate_.MayCheckClientDownload(item.get()));

  auto write_item = CreateWriteItem(target);
  EXPECT_TRUE(delegate_.MayCheckFileSystemAccessWrite(write_item.get()));
}

// The parent gate still applies when the Brave pref is on: disabling upstream
// Safe Browsing makes the delegated check return false, proving the override
// does not unconditionally return true.
TEST_F(BraveDownloadProtectionDelegateDesktopTest,
       PrefOnStillRespectsUpstreamGate) {
  SetBravePref(true);
  profile_->GetPrefs()->SetBoolean(prefs::kSafeBrowsingEnabled, false);

  const base::FilePath target(FILE_PATH_LITERAL("file.exe"));
  auto item = CreateDownloadItem(target);

  EXPECT_FALSE(delegate_.ShouldCheckDownloadUrl(item.get()));
  EXPECT_FALSE(delegate_.MayCheckClientDownload(item.get()));

  auto write_item = CreateWriteItem(target);
  EXPECT_FALSE(delegate_.MayCheckFileSystemAccessWrite(write_item.get()));
}

}  // namespace safe_browsing
