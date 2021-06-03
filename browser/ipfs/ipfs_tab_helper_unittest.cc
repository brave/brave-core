/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_tab_helper.h"

#include "brave/components/ipfs/ipfs_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/channel_info.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/version_info/channel.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ipfs {

class IpfsTabHelperUnitTest : public testing::Test {
 public:
  IpfsTabHelperUnitTest()
      : profile_manager_(TestingBrowserProcess::GetGlobal()) {}
  ~IpfsTabHelperUnitTest() override = default;

  void SetUp() override {
    ASSERT_TRUE(profile_manager_.SetUp());
    profile_ = profile_manager_.CreateTestingProfile("TestProfile");
  }
  TestingProfile* profile() { return profile_; }

 private:
  content::BrowserTaskEnvironment task_environment_;
  TestingProfileManager profile_manager_;
  TestingProfile* profile_;
};

TEST_F(IpfsTabHelperUnitTest, CanResolveURLTest) {
  std::unique_ptr<content::WebContents> web_contents(
      content::WebContentsTester::CreateTestWebContents(profile(), nullptr));
  ASSERT_TRUE(
      ipfs::IPFSTabHelper::MaybeCreateForWebContents(web_contents.get()));
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(web_contents.get());
  ASSERT_TRUE(helper);
  ASSERT_FALSE(helper->CanResolveURL(GURL("ipfs://balblabal")));
  ASSERT_FALSE(helper->CanResolveURL(GURL("file://aa")));
  ASSERT_TRUE(helper->CanResolveURL(GURL("http://a.com")));
  ASSERT_TRUE(helper->CanResolveURL(GURL("https://a.com")));

  GURL api_server = ipfs::GetAPIServer(chrome::GetChannel());
  ASSERT_FALSE(helper->CanResolveURL(api_server));
  ASSERT_TRUE(helper->CanResolveURL(GURL("https://bafyb.ipfs.dweb.link/")));
}

}  // namespace ipfs
