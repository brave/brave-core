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
#include "testing/gmock/include/gmock/gmock.h"
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
  EXPECT_THAT(info->FindBool("blocked"), testing::Optional(true));
  EXPECT_THAT(info->FindBool("didMatchRule"), testing::Optional(true));
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
  EXPECT_THAT(info->FindString("requestUrl"), testing::Pointee(url.spec()));
  EXPECT_THAT(info->FindString("resourceType"),
              testing::Pointee(testing::StrEq("Document")));
  EXPECT_THAT(info->FindBool("blocked"), testing::Optional(true));
  EXPECT_THAT(info->FindBool("didMatchRule"), testing::Optional(true));

  const auto* filter = info->FindDict("filter");
  ASSERT_TRUE(filter);
  EXPECT_THAT(filter->FindString("rawLine"), testing::Pointee(rule));
  EXPECT_THAT(filter->FindInt("lineNumber"), testing::Optional(0));
  // The test list declares no `! Title:` line.
  EXPECT_FALSE(filter->FindString("sourceTitle"));
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
  EXPECT_THAT(info->FindString("requestUrl"),
              testing::Pointee(image_url.spec()));
  EXPECT_THAT(info->FindString("resourceType"),
              testing::Pointee(testing::StrEq("Image")));
  EXPECT_THAT(info->FindBool("blocked"), testing::Optional(true));
  EXPECT_THAT(info->FindBool("didMatchRule"), testing::Optional(true));

  const auto* filter = info->FindDict("filter");
  ASSERT_TRUE(filter);
  EXPECT_THAT(filter->FindString("rawLine"), testing::Pointee(rule));
  // Custom filters are prefixed with a title comment line.
  EXPECT_THAT(filter->FindInt("lineNumber"), testing::Optional(1));
  EXPECT_THAT(filter->FindString("sourceTitle"),
              testing::Pointee(testing::StrEq("User-defined custom filters")));
  EXPECT_FALSE(info->FindDict("exception"));
}

IN_PROC_BROWSER_TEST_F(AdblockDevtoolsDebugModeTest, Exception) {
  AttachToWebContents(web_contents());
  SendCommandSync("Network.enable");

  const GURL& url = embedded_test_server()->GetURL("/blocking.html");
  const std::string filter_rule = "*ad_banner.png";
  const std::string exception_rule = "@@ad_banner.png";
  UpdateAdBlockInstanceWithRules(
      base::StrCat({"! Title: Test default filters\n", filter_rule}));
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
  EXPECT_THAT(info->FindString("requestUrl"),
              testing::Pointee(image_url.spec()));
  EXPECT_THAT(info->FindString("resourceType"),
              testing::Pointee(testing::StrEq("Image")));
  EXPECT_THAT(info->FindBool("blocked"), testing::Optional(false));
  EXPECT_THAT(info->FindBool("didMatchRule"), testing::Optional(true));
  EXPECT_THAT(info->FindBool("didMatchException"), testing::Optional(true));

  const auto* filter = info->FindDict("filter");
  ASSERT_TRUE(filter);
  EXPECT_THAT(filter->FindString("rawLine"), testing::Pointee(filter_rule));
  EXPECT_THAT(filter->FindInt("lineNumber"), testing::Optional(1));
  EXPECT_THAT(filter->FindString("sourceTitle"),
              testing::Pointee(testing::StrEq("Test default filters")));

  const auto* exception = info->FindDict("exception");
  ASSERT_TRUE(exception);
  EXPECT_THAT(exception->FindString("rawLine"),
              testing::Pointee(exception_rule));
  // Custom filters are prefixed with a title comment line.
  EXPECT_THAT(exception->FindInt("lineNumber"), testing::Optional(1));
  EXPECT_THAT(exception->FindString("sourceTitle"),
              testing::Pointee(testing::StrEq("User-defined custom filters")));
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
