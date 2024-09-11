/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/values.h"
#include "brave/browser/brave_shields/ad_block_service_browsertest.h"
#include "chrome/browser/interstitials/security_interstitial_page_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_devtools_protocol_client.h"
#include "url/gurl.h"

#if BUILDFLAG(IS_ANDROID)
#include "chrome/test/base/android/android_browser_test.h"
#else
#include "chrome/test/base/in_process_browser_test.h"
#endif

class AdblockDevtoolsTest : public AdBlockServiceTest,
                            public content::TestDevToolsProtocolClient {
 public:
  AdblockDevtoolsTest() = default;
  ~AdblockDevtoolsTest() override = default;

  bool IsShowingInterstitial() {
    return chrome_browser_interstitials::IsShowingInterstitial(web_contents());
  }

 protected:
  void TearDownOnMainThread() override {
    DetachProtocolClient();
    AdBlockServiceTest::TearDownOnMainThread();
  }
};

IN_PROC_BROWSER_TEST_F(AdblockDevtoolsTest, DomainBlock) {
  AttachToWebContents(web_contents());
  SendCommandSync("Network.enable");

  const GURL& url = embedded_test_server()->GetURL("a.com", "/simple.html");
  UpdateAdBlockInstanceWithRules("||" + url.host() + "^");
  NavigateToURL(url);

  EXPECT_TRUE(IsShowingInterstitial());

  const base::Value::Dict& notification =
      WaitForNotification("Network.requestAdblockInfoReceived", true);
  const auto* info = notification.FindDict("info");
  ASSERT_TRUE(info);
  EXPECT_EQ(url.spec(), *info->FindString("requestUrl"));
  EXPECT_EQ("Document", *info->FindString("resourceType"));
  EXPECT_TRUE(*info->FindBool("blocked"));
  EXPECT_TRUE(*info->FindBool("didMatchRule"));
}

IN_PROC_BROWSER_TEST_F(AdblockDevtoolsTest, ResourceBlock) {
  AttachToWebContents(web_contents());
  SendCommandSync("Network.enable");

  const GURL& url = embedded_test_server()->GetURL("/blocking.html");
  UpdateCustomAdBlockInstanceWithRules("*ad_banner.png");
  NavigateToURL(url);

  ASSERT_EQ(true, EvalJs(web_contents(),
                         "setExpectations(0, 1, 0, 0);"
                         "addImage('ad_banner.png')"));
  const base::Value::Dict& notification =
      WaitForNotification("Network.requestAdblockInfoReceived", true);

  const auto* info = notification.FindDict("info");
  ASSERT_TRUE(info);
  const GURL& image_url = embedded_test_server()->GetURL("/ad_banner.png");
  EXPECT_EQ(image_url.spec(), *info->FindString("requestUrl"));
  EXPECT_EQ("Image", *info->FindString("resourceType"));
  EXPECT_TRUE(*info->FindBool("blocked"));
  EXPECT_TRUE(*info->FindBool("didMatchRule"));
}

IN_PROC_BROWSER_TEST_F(AdblockDevtoolsTest, Exception) {
  AttachToWebContents(web_contents());
  SendCommandSync("Network.enable");

  const GURL& url = embedded_test_server()->GetURL("/blocking.html");
  UpdateAdBlockInstanceWithRules("*ad_banner.png");
  UpdateCustomAdBlockInstanceWithRules("@@ad_banner.png");
  NavigateToURL(url);

  ASSERT_EQ(true, EvalJs(web_contents(),
                         "setExpectations(1, 0, 0, 0);"
                         "addImage('ad_banner.png')"));
  const base::Value::Dict& notification =
      WaitForNotification("Network.requestAdblockInfoReceived", true);

  const auto* info = notification.FindDict("info");
  ASSERT_TRUE(info);
  const GURL& image_url = embedded_test_server()->GetURL("/ad_banner.png");
  EXPECT_EQ(image_url.spec(), *info->FindString("requestUrl"));
  EXPECT_EQ("Image", *info->FindString("resourceType"));
  EXPECT_FALSE(*info->FindBool("blocked"));
  EXPECT_TRUE(*info->FindBool("didMatchRule"));
  EXPECT_TRUE(*info->FindBool("didMatchException"));
}
