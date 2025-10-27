/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/json/json_reader.h"
#include "brave/browser/ui/webui/brave_settings_ui.h"
#include "brave/components/email_aliases/email_aliases_api.h"
#include "brave/components/email_aliases/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "services/network/public/cpp/network_switches.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace email_aliases {

constexpr char kSuccessEmail[] = "success@domain.com";
constexpr char kForbiddenEmail[] = "forbidden@domain.com";
constexpr char kFailEmail[] = "fail@domain.com";

namespace {

std::unique_ptr<net::test_server::HttpResponse> AuthenticationHandler(
    const net::test_server::HttpRequest& request) {
  if (!request.GetURL().has_path() ||
      !request.GetURL().path_piece().starts_with("/v2/verify/init")) {
    return nullptr;
  }

  const auto& content = base::JSONReader::Read(request.content);
  const auto data = AuthenticationRequest::FromValue(content->GetDict());

  auto response = std::make_unique<net::test_server::BasicHttpResponse>();
  response->set_code(net::HTTP_BAD_REQUEST);

  if (data && data->intent == "auth_token" &&
      data->service == "email-aliases") {
    if (data->email == kSuccessEmail) {
      response->set_code(net::HTTP_OK);
      response->set_content_type("application/json");
      response->set_content(R"({"verificationToken", "success_token"})");
    } else if (data->email == kFailEmail) {
      response->set_code(net::HTTP_OK);
      response->set_content_type("application/json");
      response->set_content(R"({"verificationToken", "fail_token"})");

    } else if (data->email == kForbiddenEmail) {
      response->set_code(net::HTTP_FORBIDDEN);
      response->set_content_type("application/json");
      response->set_content(R"json(
        {
          "code": 90001,
          "message": "service not available in user's region",
          "status": 403
        }
      )json");
    }
  }
  return response;
}

std::unique_ptr<net::test_server::HttpResponse> SessionHandler(
    const net::test_server::HttpRequest& request) {
  if (!request.GetURL().has_path() ||
      !request.GetURL().path_piece().starts_with("/v2/verify/result")) {
    return nullptr;
  }

  if (auto* authorization = base::FindOrNull(request.headers, "Authorization");
      authorization && authorization->starts_with("Bearer ")) {
    const auto token = authorization->substr(7);
    auto response = std::make_unique<net::test_server::BasicHttpResponse>();
    response->set_code(net::HTTP_BAD_REQUEST);
    if (token == "success_token") {
      response->set_code(net::HTTP_OK);
      email_aliases::SessionResponse session;
      session.auth_token = token;
      session.email = kSuccessEmail;
      session.service = "email-aliases";
      session.verified = true;
      response->set_content_type("application/json");
      response->set_content(session.ToValue().DebugString());
    } else if (token == "fail_token") {
      response->set_code(net::HTTP_UNAUTHORIZED);
      email_aliases::ErrorResponse error;
      response->set_content_type("application/json");
      response->set_content(error.ToValue().DebugString());
    } else {
      response->set_code(net::HTTP_BAD_REQUEST);
      email_aliases::ErrorResponse error;
      response->set_content_type("application/json");
      response->set_content(error.ToValue().DebugString());
    }
    return response;
  }

  return nullptr;
}

}  // namespace

class EmailAliasesBrowserTest : public InProcessBrowserTest {
 public:
  EmailAliasesBrowserTest() {
    feature_list_.InitAndEnableFeature(email_aliases::features::kEmailAliases);
    BraveSettingsUI::ShouldExposeElementsForTesting() = true;
  }

  ~EmailAliasesBrowserTest() override {
    BraveSettingsUI::ShouldExposeElementsForTesting() = false;
  }

  void SetUpOnMainThread() override {
    // Add handler for YouTube player endpoint
    https_server_.RegisterRequestHandler(
        base::BindRepeating(&AuthenticationHandler));
    https_server_.RegisterRequestHandler(base::BindRepeating(&SessionHandler));

    https_server_.StartAcceptingConnections();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
  }

  void SetUp() override {
    ASSERT_TRUE(https_server_.InitializeAndListen());
    InProcessBrowserTest::SetUp();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(
        network::switches::kHostResolverRules,
        "MAP * " + https_server_.host_port_pair().ToString());
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  content::WebContents* ActiveWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  content::WebContents* Navigate(const GURL& url) {
    ui_test_utils::NavigateToURLWithDisposition(
        browser(), url, WindowOpenDisposition::CURRENT_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
    return ActiveWebContents();
  }

  void Wait(const std::string& id) {
    constexpr const char kScript[] = R"js(
      (async () => {
        let waiter = () => {
          return !window.testing.emailAliases.getElementById($1)
        };
        while (waiter()) {
          await new Promise(r => setTimeout(r, 10));
        }
        return true;
      })();
    )js";

    ASSERT_EQ(true, content::EvalJs(ActiveWebContents(),
                                    content::JsReplace(kScript, id)));
  }

  void SetText(const std::string& id, const std::string& text) {
    constexpr char kSetText[] = R"js(
      (() => {
        const element = window.testing.emailAliases.getElementById($1);
        element.value = $2;
        element.dispatchEvent(new Event('input', {bubbles: true}));
        element.dispatchEvent(new Event('change', {bubbles: true}));
        return true;
      })();
    )js";
    ASSERT_EQ(true, content::EvalJs(ActiveWebContents(),
                                    content::JsReplace(kSetText, id, text)));
  }

  void Click(const std::string& id) {
    constexpr char kSetText[] = R"js(
      const element = window.testing.emailAliases.getElementById($1);
      element.click();
    )js";
    ASSERT_TRUE(
        content::ExecJs(ActiveWebContents(), content::JsReplace(kSetText, id)));
  }

 private:
  base::test::ScopedFeatureList feature_list_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::EmbeddedTestServer https_server_{net::EmbeddedTestServer::TYPE_HTTPS};
};

IN_PROC_BROWSER_TEST_F(EmailAliasesBrowserTest, Login) {
  Navigate(GURL("chrome://settings/email-aliases"));

  Wait("email-input");
  SetText("email-input", kSuccessEmail);
  Click("get-login-link");
}

}  // namespace email_aliases
