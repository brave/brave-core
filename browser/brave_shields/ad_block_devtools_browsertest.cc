/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/command_line.h"
#include "base/strings/strcat.h"
#include "base/values.h"
#include "brave/browser/brave_shields/ad_block_service_browsertest.h"
#include "brave/components/constants/brave_switches.h"
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

IN_PROC_BROWSER_TEST_F(AdblockDevtoolsTest, NoFilterSourceInfoByDefault) {
  AttachToWebContents(web_contents());
  SendCommandSync("Network.enable");

  const GURL& url = embedded_test_server()->GetURL("a.com", "/simple.html");
  UpdateAdBlockInstanceWithRules(base::StrCat({"||", url.host(), "^"}));
  NavigateToURL(url);

  EXPECT_TRUE(IsShowingInterstitial());

  const base::DictValue& notification =
      WaitForNotification("Network.requestAdblockInfoReceived", true);
  const auto* info = notification.FindDict("info");
  ASSERT_TRUE(info);
  EXPECT_TRUE(*info->FindBool("blocked"));
  EXPECT_TRUE(*info->FindBool("didMatchRule"));
  EXPECT_FALSE(info->FindDict("filter"));
  EXPECT_FALSE(info->FindDict("exception"));
}

class AdblockDevtoolsDebugModeTest : public AdblockDevtoolsTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    AdblockDevtoolsTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(switches::kEnableAdblockDebugMode);
  }
};

IN_PROC_BROWSER_TEST_F(AdblockDevtoolsDebugModeTest, DomainBlock) {
  AttachToWebContents(web_contents());
  SendCommandSync("Network.enable");

  const GURL& url = embedded_test_server()->GetURL("a.com", "/simple.html");
  const std::string rule = base::StrCat({"||", url.host(), "^"});
  UpdateAdBlockInstanceWithRules(rule);
  NavigateToURL(url);

  EXPECT_TRUE(IsShowingInterstitial());

  const base::DictValue& notification =
      WaitForNotification("Network.requestAdblockInfoReceived", true);
  const auto* info = notification.FindDict("info");
  ASSERT_TRUE(info);
  EXPECT_EQ(url.spec(), *info->FindString("requestUrl"));
  EXPECT_EQ("Document", *info->FindString("resourceType"));
  EXPECT_TRUE(*info->FindBool("blocked"));
  EXPECT_TRUE(*info->FindBool("didMatchRule"));

  const auto* filter = info->FindDict("filter");
  ASSERT_TRUE(filter);
  EXPECT_EQ(rule, *filter->FindString("rawLine"));
  EXPECT_EQ(0, *filter->FindInt("lineNumber"));
  EXPECT_FALSE(info->FindDict("exception"));
}

IN_PROC_BROWSER_TEST_F(AdblockDevtoolsDebugModeTest, ResourceBlock) {
  AttachToWebContents(web_contents());
  SendCommandSync("Network.enable");

  const GURL& url = embedded_test_server()->GetURL("/blocking.html");
  const std::string rule = "*resource.png";
  UpdateCustomAdBlockInstanceWithRules(rule);
  NavigateToURL(url);

  ASSERT_EQ(true, EvalJs(web_contents(),
                         "setExpectations(0, 1, 0, 0);"
                         "addImage('resource.png')"));
  const base::DictValue& notification =
      WaitForNotification("Network.requestAdblockInfoReceived", true);

  const auto* info = notification.FindDict("info");
  ASSERT_TRUE(info);
  const GURL& image_url = embedded_test_server()->GetURL("/resource.png");
  EXPECT_EQ(image_url.spec(), *info->FindString("requestUrl"));
  EXPECT_EQ("Image", *info->FindString("resourceType"));
  EXPECT_TRUE(*info->FindBool("blocked"));
  EXPECT_TRUE(*info->FindBool("didMatchRule"));

  const auto* filter = info->FindDict("filter");
  ASSERT_TRUE(filter);
  EXPECT_EQ(rule, *filter->FindString("rawLine"));
  // Custom filters are prefixed with a title comment line.
  EXPECT_EQ(1, *filter->FindInt("lineNumber"));
  EXPECT_FALSE(info->FindDict("exception"));
}

IN_PROC_BROWSER_TEST_F(AdblockDevtoolsDebugModeTest, Exception) {
  AttachToWebContents(web_contents());
  SendCommandSync("Network.enable");

  const GURL& url = embedded_test_server()->GetURL("/blocking.html");
  const std::string filter_rule = "*ad_banner.png";
  const std::string exception_rule = "@@ad_banner.png";
  UpdateAdBlockInstanceWithRules(filter_rule);
  UpdateCustomAdBlockInstanceWithRules(exception_rule);
  NavigateToURL(url);

  ASSERT_EQ(true, EvalJs(web_contents(),
                         "setExpectations(1, 0, 0, 0);"
                         "addImage('ad_banner.png')"));
  const base::DictValue& notification =
      WaitForNotification("Network.requestAdblockInfoReceived", true);

  const auto* info = notification.FindDict("info");
  ASSERT_TRUE(info);
  const GURL& image_url = embedded_test_server()->GetURL("/ad_banner.png");
  EXPECT_EQ(image_url.spec(), *info->FindString("requestUrl"));
  EXPECT_EQ("Image", *info->FindString("resourceType"));
  EXPECT_FALSE(*info->FindBool("blocked"));
  EXPECT_TRUE(*info->FindBool("didMatchRule"));
  EXPECT_TRUE(*info->FindBool("didMatchException"));

  const auto* filter = info->FindDict("filter");
  ASSERT_TRUE(filter);
  EXPECT_EQ(filter_rule, *filter->FindString("rawLine"));
  EXPECT_EQ(0, *filter->FindInt("lineNumber"));

  const auto* exception = info->FindDict("exception");
  ASSERT_TRUE(exception);
  EXPECT_EQ(exception_rule, *exception->FindString("rawLine"));
  // Custom filters are prefixed with a title comment line.
  EXPECT_EQ(1, *exception->FindInt("lineNumber"));
}

IN_PROC_BROWSER_TEST_F(AdblockDevtoolsDebugModeTest, TwoClientsNoCrash) {
  AttachToWebContents(web_contents());
  SendCommandSync("Network.enable");

  content::TestDevToolsProtocolClient second_devtools_client_;
  second_devtools_client_.AttachToWebContents(web_contents());
  second_devtools_client_.SendCommandSync("Network.enable");

  const GURL& url = embedded_test_server()->GetURL("a.com", "/simple.html");
  UpdateAdBlockInstanceWithRules(base::StrCat({"||", url.host(), "^"}));
  NavigateToURL(url);

  WaitForNotification("Network.requestAdblockInfoReceived", true);

  second_devtools_client_.DetachProtocolClient();
}
