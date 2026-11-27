/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <string_view>

#include "base/command_line.h"
#include "base/strings/strcat.h"
#include "base/values.h"
#include "brave/browser/brave_shields/ad_block_service_browsertest.h"
#include "brave/components/constants/brave_switches.h"
#include "build/build_config.h"
#include "chrome/browser/interstitials/security_interstitial_page_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_devtools_protocol_client.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "url/gurl.h"

#if BUILDFLAG(IS_ANDROID)
#include "chrome/test/base/android/android_browser_test.h"
#else
#include "chrome/browser/devtools/devtools_window_testing.h"
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

#if !BUILDFLAG(IS_ANDROID)
IN_PROC_BROWSER_TEST_F(AdblockDevtoolsTest, AdblockTabRendersInfo) {
  DevToolsWindow* devtools_window =
      DevToolsWindowTesting::OpenDevToolsWindowSync(web_contents(), false);
  content::WebContents* devtools_contents =
      DevToolsWindowTesting::Get(devtools_window)->main_web_contents();

  UpdateAdBlockInstanceWithRules("*resource.png");
  NavigateToURL(embedded_test_server()->GetURL("/blocking.html"));
  ASSERT_EQ(true, EvalJs(web_contents(),
                         "setExpectations(0, 1, 0, 0);"
                         "addImage('resource.png')"));

  const std::string text = content::EvalJs(devtools_contents, R"(
    (async () => {
      const SDK = await self.runtime.loadLegacyModule('core/sdk/sdk.js');
      const Logs = await self.runtime.loadLegacyModule('models/logs/logs.js');
      const Network =
          await self.runtime.loadLegacyModule('panels/network/network.js');
      const adblockInfo = (r) => SDK.NetworkManager.NetworkManager
          .forRequest(r).dispatcher.adblockInfoForId(r.requestId());
      let request;
      while (!request) {
        request = Logs.NetworkLog.NetworkLog.instance().requests().find(
            r => r.url().endsWith('/resource.png') && adblockInfo(r));
        await new Promise(resolve => setTimeout(resolve, 50));
      }
      const itemView = await Network.NetworkPanel.NetworkPanel.instance()
          .selectAndActivateRequest(request, 'blocking');
      const adblockView = itemView.tabView('blocking');
      await adblockView.updateComplete;
      return adblockView.contentElement.textContent;
    })()
  )")
                               .ExtractString();

  EXPECT_THAT(text,
              testing::HasSubstr(
                  embedded_test_server()->GetURL("/resource.png").spec()));
}

class AdblockDevtoolsTabTest : public AdblockDevtoolsTest {
 protected:
  // Waits for the request whose URL ends with `url_suffix` to finish, then
  // returns the rendered text of its Adblock tab.
  std::string GetAdblockTabText(DevToolsWindow* window,
                                std::string_view url_suffix,
                                bool expect_adblock_info) {
    static constexpr char kScript[] = R"(
      (async () => {
        const SDK = await self.runtime.loadLegacyModule('core/sdk/sdk.js');
        const Logs = await self.runtime.loadLegacyModule('models/logs/logs.js');
        const Network =
            await self.runtime.loadLegacyModule('panels/network/network.js');
        const adblockInfo = (r) => SDK.NetworkManager.NetworkManager
            .forRequest(r).dispatcher.adblockInfoForId(r.requestId());
        let request;
        while (!request) {
          request = Logs.NetworkLog.NetworkLog.instance().requests().find(
              r => r.url().endsWith($1) && r.finished &&
                   (!$2 || adblockInfo(r)));
          await new Promise(resolve => setTimeout(resolve, 50));
        }
        const itemView = await Network.NetworkPanel.NetworkPanel.instance()
            .selectAndActivateRequest(request, 'blocking');
        const adblockView = itemView.tabView('blocking');
        await adblockView.updateComplete;
        // The empty state renders in a shadow root, which innerText skips.
        const empty = adblockView.contentElement.querySelector(
            'devtools-widget')?.shadowRoot?.querySelector('.empty-state');
        return (empty ?? adblockView.contentElement).innerText
            .replace(/\s+/g, ' ').trim();
      })()
    )";
    return content::EvalJs(
               DevToolsWindowTesting::Get(window)->main_web_contents(),
               content::JsReplace(kScript, url_suffix, expect_adblock_info))
        .ExtractString();
  }
};

class AdblockDevtoolsTabDebugModeTest : public AdblockDevtoolsTabTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    AdblockDevtoolsTabTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(switches::kEnableAdblockDebugMode);
  }
};

IN_PROC_BROWSER_TEST_F(AdblockDevtoolsTabDebugModeTest, Blocked) {
  UpdateCustomAdBlockInstanceWithRules("*resource.png");
  NavigateToURL(embedded_test_server()->GetURL("/blocking.html"));
  DevToolsWindow* window =
      DevToolsWindowTesting::OpenDevToolsWindowSync(web_contents(), true);

  ASSERT_EQ(true, EvalJs(web_contents(),
                         "setExpectations(0, 1, 0, 0);"
                         "addImage('resource.png')"));
  const std::string text = GetAdblockTabText(window, "/resource.png", true);
  EXPECT_THAT(text, testing::HasSubstr("Verdict Blocked"));
  // The fixture defaults to aggressive mode.
  EXPECT_THAT(text, testing::HasSubstr("Blocking mode Aggressive"));
  EXPECT_THAT(text, testing::HasSubstr("Matched filter Rule *resource.png "
                                       "Filter list User-defined custom "
                                       "filters (source ID"));
  EXPECT_THAT(text, testing::HasSubstr("Line 1"));
  EXPECT_THAT(text, testing::Not(testing::HasSubstr("Matched exception")));

  DevToolsWindowTesting::CloseDevToolsWindowSync(window);
}

IN_PROC_BROWSER_TEST_F(AdblockDevtoolsTabDebugModeTest, Exception) {
  UpdateAdBlockInstanceWithRules(
      "! Title: Test default filters\n*ad_banner.png");
  UpdateCustomAdBlockInstanceWithRules("@@ad_banner.png");
  NavigateToURL(embedded_test_server()->GetURL("/blocking.html"));
  DevToolsWindow* window =
      DevToolsWindowTesting::OpenDevToolsWindowSync(web_contents(), true);

  ASSERT_EQ(true, EvalJs(web_contents(),
                         "setExpectations(1, 0, 0, 0);"
                         "addImage('ad_banner.png')"));
  const std::string text = GetAdblockTabText(window, "/ad_banner.png", true);
  EXPECT_THAT(text, testing::HasSubstr("Verdict Allowed by exception"));
  EXPECT_THAT(text,
              testing::HasSubstr("Matched filter Rule *ad_banner.png Filter "
                                 "list Test default filters (source ID"));
  EXPECT_THAT(text, testing::HasSubstr(
                        "Matched exception Rule @@ad_banner.png Filter list "
                        "User-defined custom filters (source ID"));

  DevToolsWindowTesting::CloseDevToolsWindowSync(window);
}

IN_PROC_BROWSER_TEST_F(AdblockDevtoolsTabTest, RuleNeedsDebugMode) {
  DisableAggressiveMode();
  UpdateCustomAdBlockInstanceWithRules("*resource.png");
  NavigateToURL(embedded_test_server()->GetURL("/blocking.html"));
  DevToolsWindow* window =
      DevToolsWindowTesting::OpenDevToolsWindowSync(web_contents(), true);

  ASSERT_EQ(true, EvalJs(web_contents(),
                         "setExpectations(0, 1, 0, 0);"
                         "addImage('resource.png')"));
  const std::string text = GetAdblockTabText(window, "/resource.png", true);
  EXPECT_THAT(text, testing::HasSubstr("Verdict Blocked"));
  EXPECT_THAT(text, testing::HasSubstr("Blocking mode Standard"));
  EXPECT_THAT(text, testing::HasSubstr(
                        "Matched rule Turn on Debug mode in "
                        "brave://adblock-internals and restart the browser"));
  EXPECT_THAT(text, testing::Not(testing::HasSubstr("Matched filter")));

  DevToolsWindowTesting::CloseDevToolsWindowSync(window);
}

IN_PROC_BROWSER_TEST_F(AdblockDevtoolsTabTest, NoAdblockInfo) {
  NavigateToURL(embedded_test_server()->GetURL("/blocking.html"));
  DevToolsWindow* window =
      DevToolsWindowTesting::OpenDevToolsWindowSync(web_contents(), true);

  ASSERT_EQ(true, EvalJs(web_contents(),
                         "setExpectations(1, 0, 0, 0);"
                         "addImage('logo.png')"));
  EXPECT_EQ(GetAdblockTabText(window, "/logo.png", false),
            "No adblock info No adblock rule blocked or allowed this "
            "request.");

  DevToolsWindowTesting::CloseDevToolsWindowSync(window);
}
#endif  // !BUILDFLAG(IS_ANDROID)
