/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/strings/pattern.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_utils.h"
#include "net/base/features.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "services/network/public/cpp/network_switches.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

constexpr char kFetchScript[] = R"(
(async () => {
  const response = await fetch($1, {cache: 'no-store'});
  return await response.text();
})())";

std::unique_ptr<net::test_server::HttpResponse> RespondWithServerType(
    const net::test_server::HttpRequest& request) {
  GURL url = request.GetURL();
  if (url.path_piece() != "/server_type") {
    return nullptr;
  }

  auto http_response = std::make_unique<net::test_server::BasicHttpResponse>();
  http_response->set_code(net::HTTP_OK);
  http_response->set_content_type("text/plain");
  http_response->set_content(url.scheme());
  http_response->AddCustomHeader("Access-Control-Allow-Origin", "*");
  return http_response;
}

}  // namespace

class HSTSPartitioningBrowserTestBase : public InProcessBrowserTest {
 public:
  HSTSPartitioningBrowserTestBase()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    base::FilePath path;
    base::PathService::Get(chrome::DIR_TEST_DATA, &path);
    https_server_.ServeFilesFromDirectory(path);
    content::SetupCrossSiteRedirector(&https_server_);
    https_server_.RegisterRequestHandler(
        base::BindRepeating(&RespondWithServerType));
    https_server_.AddDefaultHandlers();
    embedded_test_server()->RegisterRequestHandler(
        base::BindRepeating(&RespondWithServerType));
    EXPECT_TRUE(https_server_.Start());
    EXPECT_TRUE(embedded_test_server()->Start());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(
        network::switches::kHostResolverRules,
        base::StringPrintf("MAP *:80 127.0.0.1:%d,"
                           "MAP *:443 127.0.0.1:%d",
                           embedded_test_server()->port(),
                           https_server_.port()));
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  void ExpectHSTSState(content::RenderFrameHost* rfh,
                       const std::string& host,
                       bool is_hsts) {
    auto scheme_fetch_result = content::EvalJs(
        rfh, content::JsReplace(kFetchScript, base::StrCat({"http://", host,
                                                            "/server_type"})));
    EXPECT_EQ(is_hsts ? "https" : "http", scheme_fetch_result);
  }

  void SetHSTS(content::RenderFrameHost* rfh, const std::string& host) {
    EXPECT_TRUE(content::ExecJs(
        rfh, content::JsReplace(
                 kFetchScript,
                 base::StrCat(
                     {"https://", host,
                      "/set-header?Strict-Transport-Security: "
                      "max-age%3D600000&Access-Control-Allow-Origin: %2A"}))));
  }

  void ClearHSTS(content::RenderFrameHost* rfh, const std::string& host) {
    EXPECT_TRUE(content::ExecJs(
        rfh,
        content::JsReplace(
            kFetchScript,
            base::StrCat({"https://", host,
                          "/set-header?Strict-Transport-Security: "
                          "max-age%3D0&Access-Control-Allow-Origin: %2A"}))));
  }

  void ExpectPreloadWorks() {
    for (const auto* preloaded_host : {"brave.com", "accounts.google.com"}) {
      GURL url(base::StrCat({"http://", preloaded_host, "/simple.html"}));
      auto* rfh = ui_test_utils::NavigateToURL(browser(), url);
      ASSERT_TRUE(rfh);
      EXPECT_TRUE(rfh->GetLastCommittedURL().SchemeIsCryptographic());
    }
  }

  void NetworkContextAddHSTS(const std::string& host) {
    content::StoragePartition* partition =
        browser()->profile()->GetDefaultStoragePartition();
    base::RunLoop run_loop;
    partition->GetNetworkContext()->AddHSTS(
        host, base::Time::Now() + base::Days(1), false, run_loop.QuitClosure());
    run_loop.Run();
  }

  bool NetworkContextIsHSTSActiveForHost(const std::string& host) {
    content::StoragePartition* partition =
        browser()->profile()->GetDefaultStoragePartition();
    base::RunLoop run_loop;
    bool result = false;
    partition->GetNetworkContext()->IsHSTSActiveForHost(
        host,
        base::BindLambdaForTesting([&run_loop, &result](bool is_hsts_active) {
          result = is_hsts_active;
          run_loop.Quit();
        }));
    run_loop.Run();
    return result;
  }

  base::Value::Dict NetworkContextGetHSTSState(const std::string& host) {
    content::StoragePartition* partition =
        browser()->profile()->GetDefaultStoragePartition();
    base::RunLoop run_loop;
    base::Value::Dict result;
    partition->GetNetworkContext()->GetHSTSState(
        host, base::BindLambdaForTesting(
                  [&run_loop, &result](base::Value::Dict sts_state) {
                    result = std::move(sts_state);
                    run_loop.Quit();
                  }));
    run_loop.Run();
    return result;
  }

  bool NetworkContextDeleteDynamicDataForHost(const std::string& host) {
    content::StoragePartition* partition =
        browser()->profile()->GetDefaultStoragePartition();
    base::RunLoop run_loop;
    bool result = false;
    partition->GetNetworkContext()->DeleteDynamicDataForHost(
        host,
        base::BindLambdaForTesting([&run_loop, &result](bool is_hsts_active) {
          result = is_hsts_active;
          run_loop.Quit();
        }));
    run_loop.Run();
    return result;
  }

 protected:
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::test_server::EmbeddedTestServer https_server_;
};

class HSTSPartitioningEnabledBrowserTest
    : public HSTSPartitioningBrowserTestBase {
 public:
  HSTSPartitioningEnabledBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(
        net::features::kBravePartitionHSTS);
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(HSTSPartitioningEnabledBrowserTest, HSTSIsPartitioned) {
  // Load a.com and set b.com, c.bom HSTS inside a.com.
  GURL a_com_url("http://a.com/iframe.html");
  auto* a_com_rfh = ui_test_utils::NavigateToURL(browser(), a_com_url);
  ASSERT_TRUE(a_com_rfh);
  EXPECT_EQ(a_com_rfh->GetLastCommittedURL(), a_com_url);

  ExpectHSTSState(a_com_rfh, "b.com", false);
  SetHSTS(a_com_rfh, "b.com");
  ExpectHSTSState(a_com_rfh, "b.com", true);

  ExpectHSTSState(a_com_rfh, "c.com", false);
  SetHSTS(a_com_rfh, "c.com");
  ExpectHSTSState(a_com_rfh, "c.com", true);

  // b.com iframe should be loaded via HTTPS.
  {
    ASSERT_TRUE(NavigateIframeToURL(
        content::WebContents::FromRenderFrameHost(a_com_rfh), "test",
        GURL("http://b.com/simple.html")));
    auto* b_com_inside_a_com_rfh = ChildFrameAt(a_com_rfh, 0);
    ASSERT_TRUE(b_com_inside_a_com_rfh);
    EXPECT_TRUE(
        b_com_inside_a_com_rfh->GetLastCommittedURL().SchemeIsCryptographic());
  }

  // c.com iframe should be loaded via HTTPS.
  {
    ASSERT_TRUE(NavigateIframeToURL(
        content::WebContents::FromRenderFrameHost(a_com_rfh), "test",
        GURL("http://c.com/simple.html")));
    auto* c_com_inside_a_com_rfh = ChildFrameAt(a_com_rfh, 0);
    ASSERT_TRUE(c_com_inside_a_com_rfh);
    EXPECT_TRUE(
        c_com_inside_a_com_rfh->GetLastCommittedURL().SchemeIsCryptographic());
  }

  // d.com iframe should be loaded via HTTP.
  {
    ASSERT_TRUE(NavigateIframeToURL(
        content::WebContents::FromRenderFrameHost(a_com_rfh), "test",
        GURL("http://d.com/simple.html")));
    auto* d_com_inside_a_com_rfh = ChildFrameAt(a_com_rfh, 0);
    ASSERT_TRUE(d_com_inside_a_com_rfh);
    EXPECT_FALSE(
        d_com_inside_a_com_rfh->GetLastCommittedURL().SchemeIsCryptographic());

    ExpectHSTSState(d_com_inside_a_com_rfh, "b.com", true);
    ExpectHSTSState(d_com_inside_a_com_rfh, "c.com", true);
  }

  // Load b.com in another tab and expect HSTS is not applied.
  GURL b_com_url("http://b.com/iframe.html");
  auto* b_com_rfh = ui_test_utils::NavigateToURLWithDisposition(
      browser(), b_com_url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  ASSERT_TRUE(b_com_rfh);
  EXPECT_EQ(b_com_rfh->GetLastCommittedURL(), b_com_url);

  ExpectHSTSState(b_com_rfh, "b.com", false);
  ExpectHSTSState(b_com_rfh, "c.com", false);

  // c.com iframe inside b.com should be loaded via HTTP.
  {
    ASSERT_TRUE(NavigateIframeToURL(
        content::WebContents::FromRenderFrameHost(b_com_rfh), "test",
        GURL("http://c.com/simple.html")));
    auto* c_com_inside_b_com_rfh = ChildFrameAt(b_com_rfh, 0);
    ASSERT_TRUE(c_com_inside_b_com_rfh);
    EXPECT_FALSE(
        c_com_inside_b_com_rfh->GetLastCommittedURL().SchemeIsCryptographic());

    ExpectHSTSState(c_com_inside_b_com_rfh, "b.com", false);
    ExpectHSTSState(c_com_inside_b_com_rfh, "c.com", false);
  }

  // No data should be available via unpartitioned API.
  EXPECT_FALSE(NetworkContextIsHSTSActiveForHost("a.com"));
  EXPECT_FALSE(NetworkContextIsHSTSActiveForHost("b.com"));
  EXPECT_FALSE(NetworkContextIsHSTSActiveForHost("c.com"));
}

IN_PROC_BROWSER_TEST_F(HSTSPartitioningEnabledBrowserTest,
                       HSTSIsPartitionedUsingRegistrableDomain) {
  // Load a.com and set b.com HSTS inside a.com.
  GURL a_com_url("http://a.com/simple.html");
  auto* a_com_rfh = ui_test_utils::NavigateToURL(browser(), a_com_url);
  ASSERT_TRUE(a_com_rfh);
  EXPECT_EQ(a_com_rfh->GetLastCommittedURL(), a_com_url);

  SetHSTS(a_com_rfh, "b.com");
  ExpectHSTSState(a_com_rfh, "b.com", true);

  // Load sub.a.com, expect b.com HSTS state is applied.
  GURL sub_a_com_url("http://sub.a.com/simple.html");
  auto* sub_a_com_rfh = ui_test_utils::NavigateToURLWithDisposition(
      browser(), sub_a_com_url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  ASSERT_TRUE(sub_a_com_rfh);

  ExpectHSTSState(sub_a_com_rfh, "b.com", true);

  // No data should be available via unpartitioned API.
  EXPECT_FALSE(NetworkContextIsHSTSActiveForHost("b.com"));
}

IN_PROC_BROWSER_TEST_F(HSTSPartitioningEnabledBrowserTest,
                       HSTSIsUsedOnMainFrameLoad) {
  // Load a.com and set a.com HSTS.
  GURL a_com_url("http://a.com/simple.html");
  auto* a_com_rfh = ui_test_utils::NavigateToURL(browser(), a_com_url);
  ASSERT_TRUE(a_com_rfh);
  EXPECT_EQ(a_com_rfh->GetLastCommittedURL(), a_com_url);

  SetHSTS(a_com_rfh, "a.com");
  ExpectHSTSState(a_com_rfh, "a.com", true);

  // Load a.com in another tab, expect HSTS is applied.
  auto* a_com_rfh2 = ui_test_utils::NavigateToURLWithDisposition(
      browser(), a_com_url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  ASSERT_TRUE(a_com_rfh2);
  EXPECT_TRUE(a_com_rfh2->GetLastCommittedURL().SchemeIsCryptographic());

  // Unpartitioned API should see a.com HSTS state.
  EXPECT_TRUE(NetworkContextIsHSTSActiveForHost("a.com"));
}

IN_PROC_BROWSER_TEST_F(HSTSPartitioningEnabledBrowserTest, HSTSIsCleared) {
  // Load a.com and set c.com HSTS inside a.com.
  GURL a_com_url("http://a.com/simple.html");
  auto* a_com_rfh = ui_test_utils::NavigateToURL(browser(), a_com_url);
  ASSERT_TRUE(a_com_rfh);
  EXPECT_EQ(a_com_rfh->GetLastCommittedURL(), a_com_url);

  ExpectHSTSState(a_com_rfh, "c.com", false);
  SetHSTS(a_com_rfh, "c.com");
  ExpectHSTSState(a_com_rfh, "c.com", true);

  // Load b.com in another tab and expect c.com HSTS is not applied.
  GURL b_com_url("http://b.com/simple.html");
  auto* b_com_rfh = ui_test_utils::NavigateToURLWithDisposition(
      browser(), b_com_url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  ASSERT_TRUE(b_com_rfh);
  EXPECT_EQ(b_com_rfh->GetLastCommittedURL(), b_com_url);

  // Set c.com HSTS inside b.com.
  ExpectHSTSState(b_com_rfh, "c.com", false);
  SetHSTS(b_com_rfh, "c.com");
  ExpectHSTSState(b_com_rfh, "c.com", true);

  // Clear c.com HSTS inside a.com.
  ClearHSTS(a_com_rfh, "c.com");
  ExpectHSTSState(a_com_rfh, "c.com", false);

  // Expect c.com HSTS is still active inside b.com.
  ExpectHSTSState(b_com_rfh, "c.com", true);
}

IN_PROC_BROWSER_TEST_F(HSTSPartitioningEnabledBrowserTest,
                       PRE_HSTSIsPersisted) {
  GURL a_com_url("http://a.com/simple.html");
  auto* a_com_rfh = ui_test_utils::NavigateToURL(browser(), a_com_url);
  ASSERT_TRUE(a_com_rfh);
  EXPECT_EQ(a_com_rfh->GetLastCommittedURL(), a_com_url);

  ExpectHSTSState(a_com_rfh, "b.com", false);
  SetHSTS(a_com_rfh, "b.com");
  ExpectHSTSState(a_com_rfh, "b.com", true);
}

IN_PROC_BROWSER_TEST_F(HSTSPartitioningEnabledBrowserTest, HSTSIsPersisted) {
  GURL a_com_url("http://a.com/simple.html");
  auto* a_com_rfh = ui_test_utils::NavigateToURL(browser(), a_com_url);
  ASSERT_TRUE(a_com_rfh);
  EXPECT_EQ(a_com_rfh->GetLastCommittedURL(), a_com_url);

  ExpectHSTSState(a_com_rfh, "b.com", true);
}

IN_PROC_BROWSER_TEST_F(HSTSPartitioningEnabledBrowserTest,
                       HSTSIsIgnoredOnIpAddress) {
  GURL ip_url = embedded_test_server()->GetURL(
      "/set-header?Strict-Transport-Security: max-age%3D600000");
  auto* ip_rfh = ui_test_utils::NavigateToURL(browser(), ip_url);
  ASSERT_TRUE(ip_rfh);
}

IN_PROC_BROWSER_TEST_F(HSTSPartitioningEnabledBrowserTest,
                       HSTSIsStoredInSandbox) {
  GURL a_com_url(
      "http://a.com/set-header?"
      "Content-Security-Policy: sandbox allow-scripts");
  auto* a_com_rfh = ui_test_utils::NavigateToURL(browser(), a_com_url);
  ASSERT_TRUE(a_com_rfh);

  ExpectHSTSState(a_com_rfh, "a.com", false);
  SetHSTS(a_com_rfh, "a.com");
  ExpectHSTSState(a_com_rfh, "a.com", true);

  ExpectHSTSState(a_com_rfh, "b.com", false);
  SetHSTS(a_com_rfh, "b.com");
  ExpectHSTSState(a_com_rfh, "b.com", true);

  // Load a.com in another tab, expect HSTS is applied.
  auto* a_com_rfh2 = ui_test_utils::NavigateToURLWithDisposition(
      browser(), a_com_url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  ASSERT_TRUE(a_com_rfh2);
  EXPECT_TRUE(a_com_rfh2->GetLastCommittedURL().SchemeIsCryptographic());
}

IN_PROC_BROWSER_TEST_F(HSTSPartitioningEnabledBrowserTest,
                       HSTSIsSetIn3pIframe) {
  // Load a.com and set c.com HSTS inside b.com iframe.
  GURL a_com_url("http://a.com/iframe.html");
  auto* a_com_rfh = ui_test_utils::NavigateToURL(browser(), a_com_url);
  ASSERT_TRUE(a_com_rfh);
  EXPECT_EQ(a_com_rfh->GetLastCommittedURL(), a_com_url);

  ASSERT_TRUE(
      NavigateIframeToURL(content::WebContents::FromRenderFrameHost(a_com_rfh),
                          "test", GURL("http://b.com/simple.html")));
  auto* b_com_inside_a_com_rfh = ChildFrameAt(a_com_rfh, 0);
  ASSERT_TRUE(b_com_inside_a_com_rfh);

  ExpectHSTSState(b_com_inside_a_com_rfh, "c.com", false);
  SetHSTS(b_com_inside_a_com_rfh, "c.com");
  ExpectHSTSState(b_com_inside_a_com_rfh, "c.com", true);

  // Expect c.com HSTS state is also available in the main frame.
  ExpectHSTSState(a_com_rfh, "c.com", true);

  // No data should be available via unpartitioned API.
  EXPECT_FALSE(NetworkContextIsHSTSActiveForHost("a.com"));
  EXPECT_FALSE(NetworkContextIsHSTSActiveForHost("b.com"));
  EXPECT_FALSE(NetworkContextIsHSTSActiveForHost("c.com"));
}

IN_PROC_BROWSER_TEST_F(HSTSPartitioningEnabledBrowserTest,
                       HSTSIsSetInMainFrameOnNavigation) {
  GURL a_com_set_hsts_url(
      "https://a.com/set-header?Strict-Transport-Security: max-age%3D600000");
  auto* a_com_set_hsts_rfh =
      ui_test_utils::NavigateToURL(browser(), a_com_set_hsts_url);
  ASSERT_TRUE(a_com_set_hsts_rfh);
  EXPECT_EQ(a_com_set_hsts_rfh->GetLastCommittedURL(), a_com_set_hsts_url);

  GURL a_com_url("http://a.com/simple.html");
  auto* a_com_rfh = ui_test_utils::NavigateToURLWithDisposition(
      browser(), a_com_url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  ASSERT_TRUE(a_com_rfh);
  EXPECT_TRUE(a_com_rfh->GetLastCommittedURL().SchemeIsCryptographic());

  EXPECT_TRUE(NetworkContextIsHSTSActiveForHost("a.com"));
}

IN_PROC_BROWSER_TEST_F(HSTSPartitioningEnabledBrowserTest, HSTSPreloadWorks) {
  ExpectPreloadWorks();
}

IN_PROC_BROWSER_TEST_F(HSTSPartitioningEnabledBrowserTest, NetworkContextAPI) {
  // Load a.com and set b.com, c.bom HSTS inside a.com.
  GURL a_com_url("http://a.com/iframe.html");
  auto* a_com_rfh = ui_test_utils::NavigateToURL(browser(), a_com_url);
  ASSERT_TRUE(a_com_rfh);
  EXPECT_EQ(a_com_rfh->GetLastCommittedURL(), a_com_url);

  SetHSTS(a_com_rfh, "a.com");
  ExpectHSTSState(a_com_rfh, "a.com", true);
  EXPECT_TRUE(NetworkContextIsHSTSActiveForHost("a.com"));
  EXPECT_TRUE(
      NetworkContextGetHSTSState("a.com").FindInt("dynamic_upgrade_mode"));

  SetHSTS(a_com_rfh, "b.com");
  ExpectHSTSState(a_com_rfh, "b.com", true);
  EXPECT_FALSE(NetworkContextIsHSTSActiveForHost("b.com"));
  EXPECT_FALSE(
      NetworkContextGetHSTSState("b.com").FindInt("dynamic_upgrade_mode"));

  GURL b_com_url("http://b.com/iframe.html");
  auto* b_com_rfh = ui_test_utils::NavigateToURLWithDisposition(
      browser(), b_com_url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  ASSERT_TRUE(b_com_rfh);
  EXPECT_EQ(b_com_rfh->GetLastCommittedURL(), b_com_url);

  SetHSTS(b_com_rfh, "a.com");
  ExpectHSTSState(b_com_rfh, "a.com", true);
  SetHSTS(b_com_rfh, "b.com");
  ExpectHSTSState(b_com_rfh, "b.com", true);

  EXPECT_TRUE(NetworkContextDeleteDynamicDataForHost("a.com"));
  ExpectHSTSState(a_com_rfh, "a.com", false);
  ExpectHSTSState(b_com_rfh, "a.com", false);
  ExpectHSTSState(b_com_rfh, "b.com", true);
}

IN_PROC_BROWSER_TEST_F(HSTSPartitioningEnabledBrowserTest,
                       NetworkContextAddHSTS) {
  NetworkContextAddHSTS("sub.a.com");
  EXPECT_TRUE(NetworkContextIsHSTSActiveForHost("sub.a.com"));
  EXPECT_TRUE(
      NetworkContextGetHSTSState("sub.a.com").FindInt("dynamic_upgrade_mode"));

  GURL a_com_url("http://sub.a.com/iframe.html");
  auto* a_com_rfh = ui_test_utils::NavigateToURL(browser(), a_com_url);
  ASSERT_TRUE(a_com_rfh);
  EXPECT_TRUE(a_com_rfh->GetLastCommittedURL().SchemeIsCryptographic());
}

// Test fixture to ensure a.com domain partitioned inside a.com will reuse old
// format HSTS data.
class HSTSSameDomainPartitionUsesOldFormatBrowserTest
    : public HSTSPartitioningBrowserTestBase {
 public:
  HSTSSameDomainPartitionUsesOldFormatBrowserTest() {
    if (IsPreTest()) {
      scoped_feature_list_.InitAndDisableFeature(
          net::features::kBravePartitionHSTS);
    } else {
      scoped_feature_list_.InitAndEnableFeature(
          net::features::kBravePartitionHSTS);
    }
  }

  static bool IsPreTest() {
    const ::testing::TestInfo* const test_info =
        ::testing::UnitTest::GetInstance()->current_test_info();
    return std::string_view(test_info->name()).starts_with("PRE_");
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(HSTSSameDomainPartitionUsesOldFormatBrowserTest,
                       PRE_UnpartitionedHSTSIsUsed) {
  GURL a_com_url("http://a.com/simple.html");
  auto* a_com_rfh = ui_test_utils::NavigateToURL(browser(), a_com_url);
  ASSERT_TRUE(a_com_rfh);
  EXPECT_EQ(a_com_rfh->GetLastCommittedURL(), a_com_url);

  ExpectHSTSState(a_com_rfh, "a.com", false);
  // This will be stored without partitioning, becase the feature is disabled.
  // key = hash(a.com).
  SetHSTS(a_com_rfh, "a.com");
  ExpectHSTSState(a_com_rfh, "a.com", true);
  EXPECT_TRUE(NetworkContextIsHSTSActiveForHost("a.com"));

  ExpectHSTSState(a_com_rfh, "sub.b.com", false);
  // This will be stored without partitioning, becase the feature is disabled.
  // key = hash(sub.b.com).
  SetHSTS(a_com_rfh, "sub.b.com");
  ExpectHSTSState(a_com_rfh, "sub.b.com", true);
  EXPECT_TRUE(NetworkContextIsHSTSActiveForHost("sub.b.com"));
}

IN_PROC_BROWSER_TEST_F(HSTSSameDomainPartitionUsesOldFormatBrowserTest,
                       UnpartitionedHSTSIsUsed) {
  // a.com should be HSTS-enabled, because it was stored in the old format for
  // a.com host: key = hash(a.com). When partitioning is enabled and
  // hashes for the domain and for the partition are same, we use domain hash
  // directly without appending partition hash.
  // This will try to look for key = hash(a.com).
  GURL a_com_url("http://a.com/simple.html");
  auto* a_com_rfh = ui_test_utils::NavigateToURL(browser(), a_com_url);
  ASSERT_TRUE(a_com_rfh);
  EXPECT_TRUE(a_com_rfh->GetLastCommittedURL().SchemeIsCryptographic());
  EXPECT_TRUE(NetworkContextIsHSTSActiveForHost("a.com"));

  // This will try to look for key = hash(b.com).
  // It should not have HSTS enabled, because no data was stored for the key.
  GURL b_com_url("http://b.com/simple.html");
  auto* b_com_rfh = ui_test_utils::NavigateToURL(browser(), b_com_url);
  ASSERT_TRUE(b_com_rfh);
  EXPECT_FALSE(b_com_rfh->GetLastCommittedURL().SchemeIsCryptographic());
  EXPECT_FALSE(NetworkContextIsHSTSActiveForHost("b.com"));

  // This will try to look for key = hash(sub.b.com)/2 + hash(b.com)/2.
  // It should not have HSTS enabled, because no data was stored for the key.
  GURL sub_b_com_url("http://sub.b.com/simple.html");
  auto* sub_b_com_rfh = ui_test_utils::NavigateToURL(browser(), sub_b_com_url);
  ASSERT_TRUE(sub_b_com_rfh);
  EXPECT_FALSE(sub_b_com_rfh->GetLastCommittedURL().SchemeIsCryptographic());
  // This should return false, because new format looks into etldp1("sub.b.com")
  // partition, which is equal to "b.com", but no such data was stored.
  EXPECT_FALSE(NetworkContextIsHSTSActiveForHost("sub.b.com"));
}

class HSTSPartitioningDisabledBrowserTest
    : public HSTSPartitioningBrowserTestBase {
 public:
  HSTSPartitioningDisabledBrowserTest() {
    scoped_feature_list_.InitAndDisableFeature(
        net::features::kBravePartitionHSTS);
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(HSTSPartitioningDisabledBrowserTest,
                       HSTSIsNotPartitioned) {
  // Load a.com and set b.com HSTS inside a.com.
  GURL a_com_url("http://a.com/simple.html");
  auto* a_com_rfh = ui_test_utils::NavigateToURL(browser(), a_com_url);
  ASSERT_TRUE(a_com_rfh);
  EXPECT_EQ(a_com_rfh->GetLastCommittedURL(), a_com_url);

  ExpectHSTSState(a_com_rfh, "a.com", false);
  EXPECT_FALSE(NetworkContextIsHSTSActiveForHost("a.com"));

  ExpectHSTSState(a_com_rfh, "b.com", false);
  SetHSTS(a_com_rfh, "b.com");
  ExpectHSTSState(a_com_rfh, "b.com", true);
  EXPECT_TRUE(NetworkContextIsHSTSActiveForHost("b.com"));

  ExpectHSTSState(a_com_rfh, "c.com", false);
  SetHSTS(a_com_rfh, "c.com");
  ExpectHSTSState(a_com_rfh, "c.com", true);
  EXPECT_TRUE(NetworkContextIsHSTSActiveForHost("c.com"));

  // Load b.com in another tab and expect HSTS is applied.
  GURL b_com_url("http://b.com/simple.html");
  auto* b_com_rfh = ui_test_utils::NavigateToURLWithDisposition(
      browser(), b_com_url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  ASSERT_TRUE(b_com_rfh);
  EXPECT_TRUE(b_com_rfh->GetLastCommittedURL().SchemeIsCryptographic());

  // Load b.com in another tab and expect HSTS is applied.
  GURL c_com_url("http://c.com/simple.html");
  auto* c_com_rfh = ui_test_utils::NavigateToURLWithDisposition(
      browser(), c_com_url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  ASSERT_TRUE(c_com_rfh);
  EXPECT_TRUE(c_com_rfh->GetLastCommittedURL().SchemeIsCryptographic());

  // Load a.com and expect HSTS is not applied.
  auto* a_com_rfh2 = ui_test_utils::NavigateToURL(browser(), a_com_url);
  ASSERT_TRUE(a_com_rfh2);
  EXPECT_EQ(a_com_rfh2->GetLastCommittedURL(), a_com_url);
}

IN_PROC_BROWSER_TEST_F(HSTSPartitioningDisabledBrowserTest, HSTSPreloadWorks) {
  ExpectPreloadWorks();
}
