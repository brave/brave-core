/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_gateway.h"

#include <memory>

#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

class IPFSGatewayUnitTest : public testing::Test {
 public:
  IPFSGatewayUnitTest() : context_(new content::TestBrowserContext()) {}
  ~IPFSGatewayUnitTest() override = default;

  void SetUp() override {
    prefs_.registry()->RegisterStringPref(kIPFSPublicGatewayAddress,
                                          ipfs::kDefaultIPFSGateway);
    user_prefs::UserPrefs::Set(context_.get(), &prefs_);
  }

  content::TestBrowserContext* context() { return context_.get(); }

  PrefService* prefs() { return &prefs_; }

 private:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<content::TestBrowserContext> context_;
  TestingPrefServiceSimple prefs_;
};

TEST_F(IPFSGatewayUnitTest, GetDefaultIPFSGateway) {
  prefs()->SetString(kIPFSPublicGatewayAddress, "https://example.com/");
  EXPECT_EQ(ipfs::GetDefaultIPFSGateway(context()),
            GURL("https://example.com/"));
}
