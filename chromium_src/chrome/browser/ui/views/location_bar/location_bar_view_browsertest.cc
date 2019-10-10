/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/location_bar/location_bar_view.h"

#include "base/strings/string_util.h"
#include "chrome/browser/ssl/security_state_tab_helper.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/omnibox/browser/location_bar_model_impl.h"
#include "components/omnibox/common/omnibox_features.h"
#include "content/public/test/url_loader_interceptor.h"
#include "net/cert/ct_policy_status.h"
#include "net/ssl/ssl_info.h"
#include "net/test/cert_test_util.h"
#include "net/test/test_data_directory.h"
#include "services/network/public/cpp/resource_response.h"

const char kMockSecureHostname[] = "example-secure.test";
const GURL kMockSecureURL = GURL("https://example-secure.test");

class SecurityIndicatorTest : public InProcessBrowserTest {
 public:
  SecurityIndicatorTest() : InProcessBrowserTest(), cert_(nullptr) {}

  void SetUpInProcessBrowserTestFixture() override {
    cert_ =
        net::ImportCertFromFile(net::GetTestCertsDirectory(), "ok_cert.pem");
    ASSERT_TRUE(cert_);
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  LocationBarView* GetLocationBarView() {
    BrowserView* browser_view =
        BrowserView::GetBrowserViewForBrowser(browser());
    return browser_view->GetLocationBarView();
  }

  void SetUpInterceptor(net::CertStatus cert_status) {
    url_loader_interceptor_ = std::make_unique<content::URLLoaderInterceptor>(
        base::BindRepeating(&SecurityIndicatorTest::InterceptURLLoad,
                            base::Unretained(this), cert_status));
  }

  void ResetInterceptor() { url_loader_interceptor_.reset(); }

  bool InterceptURLLoad(net::CertStatus cert_status,
                        content::URLLoaderInterceptor::RequestParams* params) {
    if (params->url_request.url.host() != kMockSecureHostname)
      return false;
    net::SSLInfo ssl_info;
    ssl_info.cert = cert_;
    ssl_info.cert_status = cert_status;
    ssl_info.ct_policy_compliance =
        net::ct::CTPolicyCompliance::CT_POLICY_COMPLIES_VIA_SCTS;
    network::ResourceResponseHead resource_response;
    resource_response.mime_type = "text/html";
    resource_response.ssl_info = ssl_info;
    params->client->OnReceiveResponse(resource_response);
    // Send an empty response's body. This pipe is not filled with data.
    mojo::DataPipe pipe;
    params->client->OnStartLoadingResponseBody(std::move(pipe.consumer_handle));
    network::URLLoaderCompletionStatus completion_status;
    completion_status.ssl_info = ssl_info;
    params->client->OnComplete(completion_status);
    return true;
  }

 private:
  scoped_refptr<net::X509Certificate> cert_;
  std::unique_ptr<content::URLLoaderInterceptor> url_loader_interceptor_;

  DISALLOW_COPY_AND_ASSIGN(SecurityIndicatorTest);
};

IN_PROC_BROWSER_TEST_F(SecurityIndicatorTest, CheckIndicatorText) {
  const GURL kMockNonsecureURL =
      embedded_test_server()->GetURL("example.test", "/");
  const base::string16 kEmptyString = base::EmptyString16();

  const struct {
    GURL url;
    net::CertStatus cert_status;
    security_state::SecurityLevel security_level;
    bool should_show_text;
    base::string16 indicator_text;
  } cases[]{// Default
            {kMockSecureURL, net::CERT_STATUS_IS_EV, security_state::EV_SECURE,
             false, kEmptyString},
            {kMockSecureURL, 0, security_state::SECURE, false, kEmptyString},
            {kMockNonsecureURL, 0, security_state::NONE, false, kEmptyString}};

  content::WebContents* tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(tab);

  // After SetUpInterceptor() is called, requests to this hostname will be
  // mocked and use specified certificate validation results. This allows tests
  // to mock Extended Validation (EV) certificate connections.
  SecurityStateTabHelper* helper = SecurityStateTabHelper::FromWebContents(tab);
  ASSERT_TRUE(helper);
  LocationBarView* location_bar_view = GetLocationBarView();

  for (const auto& c : cases) {
    base::test::ScopedFeatureList scoped_feature_list;
    scoped_feature_list.InitAndEnableFeature(omnibox::kSimplifyHttpsIndicator);
    SetUpInterceptor(c.cert_status);
    ui_test_utils::NavigateToURL(browser(), c.url);
    EXPECT_EQ(c.security_level, helper->GetSecurityLevel());
    EXPECT_EQ(c.should_show_text,
              location_bar_view->location_icon_view()->ShouldShowLabel());
    EXPECT_EQ(c.indicator_text,
              location_bar_view->location_icon_view()->GetText());
    ResetInterceptor();
  }
}
