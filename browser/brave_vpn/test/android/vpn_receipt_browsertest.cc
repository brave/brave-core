/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_vpn/features.h"
#include "brave/components/brave_vpn/pref_names.h"
#include "brave/components/skus/common/features.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/test/base/android/android_browser_test.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/url_loader_interceptor.h"
#include "url/gurl.h"

namespace brave_vpn {

namespace {
std::string GetHeadersForURL(const std::string& url) {
  if (base::EndsWith(url, "redirect=true", base::CompareCase::SENSITIVE)) {
    return "HTTP/1.1 302 Found\nContent-type: text/html\nLocation: "
           "https://account.brave.com/\n\n";
  }
  return "HTTP/1.1 200 OK\nContent-type: text/html\n\n";
}

bool URLLoaderInterceptorCallback(
    content::URLLoaderInterceptor::RequestParams* params) {
  LOG(ERROR) << "spec:" << params->url_request.url.spec();
  content::URLLoaderInterceptor::WriteResponse(
      GetHeadersForURL(params->url_request.url.query()),
      "<html><body></body></html>", params->client.get());
  return true;
}
}  // namespace

class VpnReceiptBrowserTest : public PlatformBrowserTest {
 public:
  VpnReceiptBrowserTest() : PlatformBrowserTest() {
    scoped_feature_list_.InitWithFeatures(
        {skus::features::kSkusFeature, brave_vpn::features::kBraveVPN}, {});
  }
  ~VpnReceiptBrowserTest() override = default;

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    // We use a URLLoaderInterceptor, rather than the EmbeddedTestServer, since
    // the origin trial token in the response is associated with a fixed
    // origin, whereas EmbeddedTestServer serves content on a random port.
    url_loader_interceptor_ = std::make_unique<content::URLLoaderInterceptor>(
        base::BindRepeating(&URLLoaderInterceptorCallback));
  }

  void TearDown() override {
    url_loader_interceptor_.reset();
    PlatformBrowserTest::TearDown();
  }
  content::WebContents* GetWebContents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }
  Profile* profile() { return ProfileManager::GetActiveUserProfile(); }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;

 private:
  std::unique_ptr<content::URLLoaderInterceptor> url_loader_interceptor_;
};

IN_PROC_BROWSER_TEST_F(VpnReceiptBrowserTest, Receipt) {
  std::string test_token = "test";
  profile()->GetPrefs()->SetString(prefs::kBraveVPNPurchaseTokenAndroid,
                                   test_token);
  GURL url("https://account.brave.com/?intent=connect-receipt&product=vpn");
  EXPECT_TRUE(content::NavigateToURL(GetWebContents(), url));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(GetWebContents()->GetVisibleURL(), url);
  auto receipt =
      content::EvalJs(GetWebContents(),
                      "window.sessionStorage.getItem('braveVpn.receipt')")
          .ExtractString();
  EXPECT_FALSE(receipt.empty());
  std::string receipt_json;
  EXPECT_TRUE(base::Base64Decode(receipt, &receipt_json));
  EXPECT_EQ(base::JSONReader::Read(receipt_json), base::JSONReader::Read(R"({
    "package": "com.brave.browser",
    "raw_receipt": "test",
    "subscription_id": "brave-firewall-vpn-premium",
    "type": "android"
  })"));
}

IN_PROC_BROWSER_TEST_F(VpnReceiptBrowserTest, Redirect) {
  std::string test_token = "test";
  profile()->GetPrefs()->SetString(prefs::kBraveVPNPurchaseTokenAndroid,
                                   test_token);
  GURL url(
      "https://account.brave.com/"
      "?intent=connect-receipt&product=vpn&redirect=true");
  EXPECT_TRUE(content::NavigateToURL(GetWebContents(), url));
  EXPECT_TRUE(content::EvalJs(
                  GetWebContents(),
                  "window.sessionStorage.getItem('braveVpn.receipt') === null")
                  .ExtractBool());
  EXPECT_EQ(GetWebContents()->GetVisibleURL(),
            GURL("https://account.brave.com/"));
}

}  // namespace brave_vpn
