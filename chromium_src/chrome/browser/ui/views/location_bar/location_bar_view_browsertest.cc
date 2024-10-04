/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/location_bar/location_bar_view.h"

#include <optional>

#include "base/strings/string_util.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/omnibox/omnibox_view_views.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/omnibox/browser/location_bar_model_impl.h"
#include "components/omnibox/browser/omnibox_edit_model.h"
#include "components/security_state/content/security_state_tab_helper.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/url_loader_interceptor.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "net/cert/ct_policy_status.h"
#include "net/ssl/ssl_info.h"
#include "net/test/cert_test_util.h"
#include "net/test/test_data_directory.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

constexpr char kMockSecureHostname[] = "example-secure.test";
struct SecurityIndicatorTestParams {
  bool use_secure_url;
  net::CertStatus cert_status;
  security_state::SecurityLevel security_level;
  bool should_show_text;
  std::u16string indicator_text;
};

class SecurityIndicatorTest
    : public InProcessBrowserTest,
      public ::testing::WithParamInterface<SecurityIndicatorTestParams> {
 public:
  SecurityIndicatorTest() : cert_(nullptr) {}

  SecurityIndicatorTest(const SecurityIndicatorTest&) = delete;
  SecurityIndicatorTest& operator=(const SecurityIndicatorTest&) = delete;

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
    if (params->url_request.url.host() != kMockSecureHostname) {
      return false;
    }
    net::SSLInfo ssl_info;
    ssl_info.cert = cert_;
    ssl_info.cert_status = cert_status;
    ssl_info.ct_policy_compliance =
        net::ct::CTPolicyCompliance::CT_POLICY_COMPLIES_VIA_SCTS;
    auto resource_response = network::mojom::URLResponseHead::New();
    resource_response->mime_type = "text/html";
    resource_response->ssl_info = ssl_info;
    // Send an empty response's body. This pipe is not filled with data.
    mojo::ScopedDataPipeProducerHandle producer_handle;
    mojo::ScopedDataPipeConsumerHandle consumer_handle;
    mojo::CreateDataPipe(nullptr, producer_handle, consumer_handle);

    params->client->OnReceiveResponse(std::move(resource_response),
                                      std::move(consumer_handle), std::nullopt);

    network::URLLoaderCompletionStatus completion_status;
    completion_status.ssl_info = ssl_info;
    params->client->OnComplete(completion_status);
    return true;
  }

 private:
  scoped_refptr<net::X509Certificate> cert_;

  std::unique_ptr<content::URLLoaderInterceptor> url_loader_interceptor_;
};

IN_PROC_BROWSER_TEST_P(SecurityIndicatorTest, CheckIndicatorText) {
  const GURL kMockSecureURL = GURL("https://example-secure.test");
  const GURL kMockNonsecureURL =
      embedded_test_server()->GetURL("example.test", "/");

  content::WebContents* tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(tab);
  SecurityStateTabHelper* helper = SecurityStateTabHelper::FromWebContents(tab);
  ASSERT_TRUE(helper);
  LocationBarView* location_bar_view = GetLocationBarView();

  auto c = GetParam();
  SetUpInterceptor(c.cert_status);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), c.use_secure_url ? kMockSecureURL : kMockNonsecureURL));
  EXPECT_EQ(c.security_level, helper->GetSecurityLevel());
  EXPECT_EQ(c.should_show_text,
            location_bar_view->location_icon_view()->ShouldShowLabel());
  EXPECT_EQ(c.indicator_text,
            location_bar_view->location_icon_view()->GetText());
  ResetInterceptor();
}

const std::u16string kEmptyString = std::u16string();
INSTANTIATE_TEST_SUITE_P(
    /* no prefix */,
    SecurityIndicatorTest,
    ::testing::Values(
        // Default (lock-only in omnibox)
        SecurityIndicatorTestParams{true, net::CERT_STATUS_IS_EV,
                                    security_state::SECURE, false,
                                    kEmptyString},
        SecurityIndicatorTestParams{true, 0, security_state::SECURE, false,
                                    kEmptyString},
        SecurityIndicatorTestParams{false, 0, security_state::NONE, false,
                                    kEmptyString}));

class BraveLocationBarViewColorOverridesTest : public InProcessBrowserTest {
 public:
  BraveLocationBarViewColorOverridesTest() = default;
  BraveLocationBarViewColorOverridesTest(
      const BraveLocationBarViewColorOverridesTest&) = delete;
  BraveLocationBarViewColorOverridesTest& operator=(
      const BraveLocationBarViewColorOverridesTest&) = delete;
  ~BraveLocationBarViewColorOverridesTest() override = default;

  OmniboxViewViews* GetOmniboxView() {
    return BrowserView::GetBrowserViewForBrowser(browser())
        ->GetLocationBarView()
        ->omnibox_view();
  }
};

// We override the behavior of the LocationBar when the user is editing text.
// This test makes sure the override is being applied.
IN_PROC_BROWSER_TEST_F(BraveLocationBarViewColorOverridesTest,
                       UnfocusedEditingColorIsSameAsUnfocused) {
  // NTP is special, so we navigate somewhere else - this also unfocused the
  // LocationBar.
  ui_test_utils::NavigateToURLBlockUntilNavigationsComplete(
      browser(), GURL("https://example.com"), 1);
  EXPECT_FALSE(GetOmniboxView()->model()->is_caret_visible());
  EXPECT_FALSE(GetOmniboxView()->model()->user_input_in_progress());
  auto default_color = GetOmniboxView()->GetBackgroundColor();

  // Set the user text
  GetOmniboxView()->SetUserText(u"hello world");

  EXPECT_FALSE(GetOmniboxView()->model()->is_caret_visible());
  EXPECT_TRUE(GetOmniboxView()->model()->user_input_in_progress());
  EXPECT_EQ(default_color, GetOmniboxView()->GetBackgroundColor());
}
