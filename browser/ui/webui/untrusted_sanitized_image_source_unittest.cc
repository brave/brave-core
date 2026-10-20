// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/untrusted_sanitized_image_source.h"

#include "base/memory/scoped_refptr.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

class UntrustedSanitizedImageSourceTest : public testing::Test {
 public:
  UntrustedSanitizedImageSourceTest()
      : source_(
            &profile_,
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &test_url_loader_factory_),
            /*delegate=*/nullptr) {}

 protected:
  content::BrowserTaskEnvironment task_environment_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  TestingProfile profile_;
  UntrustedSanitizedImageSource source_;
};

TEST_F(UntrustedSanitizedImageSourceTest, AllowsListedUntrustedOrigins) {
  for (const char* origin : {
           "chrome-untrusted://leo-ai-conversation-entries",
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
           "chrome-untrusted://market-display",
           "chrome-untrusted://nft-display",
#endif
       }) {
    SCOPED_TRACE(origin);
    EXPECT_EQ(source_.GetAccessControlAllowOriginForOrigin(origin), origin);
  }
}

TEST_F(UntrustedSanitizedImageSourceTest, DeniesOtherOrigins) {
  for (const char* origin : {
           "chrome-untrusted://news",
           "chrome://leo-ai",
           "https://example.com",
       }) {
    SCOPED_TRACE(origin);
    EXPECT_EQ(source_.GetAccessControlAllowOriginForOrigin(origin), "");
  }
}
