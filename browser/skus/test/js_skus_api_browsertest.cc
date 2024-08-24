// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string>

#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/components/skus/browser/pref_names.h"
#include "brave/components/skus/common/features.h"
#include "chrome/browser/browser_process.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/common/content_client.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "services/network/public/cpp/network_switches.h"

namespace {

constexpr char kScriptTemplate[] = R"((async () => {
      try {
          await window.chrome.braveSkus.fetch_order_credentials("$1");
          document.title = 'success';
      } catch (error) {
          document.title = error;
      }
  })();)";

const char kSkusState[] = R"({
    "credentials": null,
    "orders": {
      "f24787ab-7bc3-46b9-bc05-65befb360cb8": {
        "created_at": "2023-10-24T16:00:57.902289",
        "currency": "USD",
        "expires_at": "2023-12-24T17:03:59.030987",
        "id": "f24787ab-7bc3-46b9-bc05-65befb360cb8",
        "items": [
          {
            "created_at": "2023-10-24T16:00:57.902289",
            "credential_type": "time-limited-v2",
            "currency": "USD",
            "description": "brave-leo-premium",
            "id": "b9114ccc-b3a5-4951-9a5d-8b7a28732054",
            "location": "leo.brave.com",
            "order_id": "f24787ab-7bc3-46b9-bc05-65befb360cb8",
            "price": 15,
            "quantity": 1,
            "sku": "brave-leo-premium",
            "subtotal": 15,
            "updated_at": "2023-10-24T16:00:57.902289"
          }
        ],
        "last_paid_at": "2023-11-24T17:03:59.030987",
        "location": "leo.brave.com",
        "merchant_id": "brave.com",
        "metadata": {
          "num_intervals": 3,
          "num_per_interval": 192,
          "payment_processor": "stripe",
          "stripe_checkout_session_id": "cs_live_b1lZu8rs8O0CvxymIK5W0zeEVhaYqq6H5SvXMwAkkv5PDxiN4g2cSGlCNH"
        },
        "status": "paid",
        "total_price": 15,
        "updated_at": "2023-11-24T17:03:59.030303"
      }
    },
    "promotions": null,
    "wallet": null
  })";

std::string StripTrailingUUID(const std::string& input) {
  const size_t UUIDLength = 36;
  if (input.length() <= UUIDLength) {
    return "";
  }

  return input.substr(0, input.length() - UUIDLength);
}

std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
    const net::test_server::HttpRequest& request) {
  const GURL url = request.GetURL();
  const std::string path_trimmed = StripTrailingUUID(url.path());
  auto response = std::make_unique<net::test_server::BasicHttpResponse>();
  if (request.method_string == "GET" &&
      path_trimmed ==
          "/v1/orders/f24787ab-7bc3-46b9-bc05-65befb360cb8/credentials/items/"
          "b9114ccc-b3a5-4951-9a5d-8b7a28732054/batches/") {
    response->set_code(net::HTTP_OK);
    response->set_content(R"([{
      "expiresAt": "2023-12-31",
      "id": "12345abcdef",
      "issuedAt": "2023-01-01",
      "orderId": "f24787ab-7bc3-46b9-bc05-65befb360cb8",
      "token": "lkjhgfdsa09876"
    }])");
    response->set_content_type("application/json");
  } else {
    response->set_code(net::HTTP_OK);
    response->set_content(R"(
                          <html>
                           <head><title>OK</title></head>
                          </html>
                        )");
    response->set_content_type("application/json");
  }

  return response;
}

}  // namespace

namespace skus {

class SkusAPIBrowserTest : public PlatformBrowserTest {
 public:
  SkusAPIBrowserTest() = default;

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    local_state_ = g_browser_process->local_state();
    base::Value::Dict state;
    state.Set("skus:development", kSkusState);
    local_state_->SetDict(skus::prefs::kSkusState, std::move(state));

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    content::SetBrowserClientForTesting(&client_);

    https_server_.RegisterRequestHandler(base::BindRepeating(HandleRequest));
    https_server_.StartAcceptingConnections();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    ASSERT_TRUE(https_server_.InitializeAndListen());
    // Add a host resolver rule to map all outgoing requests to the test server.
    // This allows us to use "real" hostnames and standard ports in URLs (i.e.,
    // without having to inject the port number into all URLs).
    command_line->AppendSwitchASCII(
        network::switches::kHostResolverRules,
        "MAP * " + https_server_.host_port_pair().ToString() +
            ",EXCLUDE localhost");
    PlatformBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    PlatformBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    PlatformBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }
  raw_ptr<PrefService> local_state_;

 protected:
  net::EmbeddedTestServer https_server_{net::EmbeddedTestServer::TYPE_HTTPS};

 private:
  BraveContentBrowserClient client_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  base::test::ScopedFeatureList scoped_feature_list_{
      skus::features::kSkusFeature};
};

IN_PROC_BROWSER_TEST_F(SkusAPIBrowserTest, FetchOrderCredentialsSuccess) {
  EXPECT_TRUE(content::NavigateToURL(web_contents(),
                                     GURL("https://account.brave.software/")));
  // Valid ID results in no errors.
  const std::string script = base::ReplaceStringPlaceholders(
      kScriptTemplate, {"f24787ab-7bc3-46b9-bc05-65befb360cb8"}, nullptr);
  content::ExecuteScriptAsync(web_contents()->GetPrimaryMainFrame(), script);
  std::u16string expected_title(u"success");
  content::TitleWatcher watcher(web_contents(), expected_title);
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}

IN_PROC_BROWSER_TEST_F(SkusAPIBrowserTest, FetchOrderCredentialsError) {
  EXPECT_TRUE(content::NavigateToURL(web_contents(),
                                     GURL("https://account.brave.software/")));
  // Invalid item id yields an error that's passed up to JS layer.
  const std::string script =
      base::ReplaceStringPlaceholders(kScriptTemplate, {""}, nullptr);
  content::ExecuteScriptAsync(web_contents()->GetPrimaryMainFrame(), script);
  std::u16string expected_title(u"Could not (de)serialize");
  content::TitleWatcher watcher(web_contents(), expected_title);
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}

}  // namespace skus
