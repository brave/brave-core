/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/json/json_reader.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "brave/browser/email_aliases/email_aliases_service_factory.h"
#include "brave/browser/ui/email_aliases/email_aliases_controller.h"
#include "brave/browser/ui/webui/brave_settings_ui.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/email_aliases/email_aliases_api.h"
#include "brave/components/email_aliases/email_aliases_service.h"
#include "brave/components/email_aliases/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu_test_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_navigation_observer.h"
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
      response->set_content(R"({"verificationToken": "success_token"})");
    } else if (data->email == kFailEmail) {
      response->set_code(net::HTTP_OK);
      response->set_content_type("application/json");
      response->set_content(R"({"verificationToken": "fail_token"})");

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
      response->set_content(*base::WriteJson(session.ToValue()));
    } else if (token == "fail_token") {
      response->set_code(net::HTTP_UNAUTHORIZED);
      email_aliases::ErrorResponse error;
      response->set_content_type("application/json");
      response->set_content(*base::WriteJson(error.ToValue()));
    } else {
      response->set_code(net::HTTP_BAD_REQUEST);
      email_aliases::ErrorResponse error;
      response->set_content_type("application/json");
      response->set_content(*base::WriteJson(error.ToValue()));
    }
    return response;
  }

  return nullptr;
}

void ExecuteNewEmailAliasCommand(RenderViewContextMenu* context_menu,
                                 base::OnceClosure quit_closure) {
  // Calls EngineConsumer::GenerateRewriteSuggestion
  context_menu->ExecuteCommand(IDC_NEW_EMAIL_ALIAS, 0);
  context_menu->Cancel();
  std::move(quit_closure).Run();
}

}  // namespace

class EmailAliasesBrowserTest : public InProcessBrowserTest {
 public:
  EmailAliasesBrowserTest() {
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
    https_server_.ServeFilesFromDirectory(
        base::PathService::CheckedGet(brave::DIR_TEST_DATA));

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

  void Wait(const std::string& id, content::WebContents* contents = nullptr) {
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

    ASSERT_EQ(true, content::EvalJs((contents ? contents : ActiveWebContents()),
                                    content::JsReplace(kScript, id)));
  }

  void SetText(const std::string& id,
               const std::string& text,
               content::WebContents* contents = nullptr) {
    constexpr char kSetText[] = R"js(
      (() => {
        const element = window.testing.emailAliases.getElementById($1);
        element.value = $2;
        element.dispatchEvent(new Event('input', {bubbles: true}));
        element.dispatchEvent(new Event('change', {bubbles: true}));
        return true;
      })();
    )js";
    ASSERT_EQ(true, content::EvalJs((contents ? contents : ActiveWebContents()),
                                    content::JsReplace(kSetText, id, text)));
  }

  void Click(const std::string& id, content::WebContents* contents = nullptr) {
    constexpr char kSetText[] = R"js(
      const element = window.testing.emailAliases.getElementById($1);
      element.click();
    )js";
    ASSERT_TRUE(content::ExecJs((contents ? contents : ActiveWebContents()),
                                content::JsReplace(kSetText, id)));
  }

  EmailAliasesService* email_aliases_service() {
    return EmailAliasesServiceFactory::GetServiceForProfile(
        browser()->profile());
  }

  void RunContextMenuOn(const std::string& element_id) {
    const int x =
        content::EvalJs(ActiveWebContents(),
                        content::JsReplace("getElementX($1)", element_id))
            .ExtractInt();
    const int y =
        content::EvalJs(ActiveWebContents(),
                        content::JsReplace("getElementY($1)", element_id))
            .ExtractInt();

    base::RunLoop loop;
    RenderViewContextMenu::RegisterMenuShownCallbackForTesting(
        base::BindLambdaForTesting([&](RenderViewContextMenu* context_menu) {
          base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
              FROM_HERE, base::BindOnce(&ExecuteNewEmailAliasCommand,
                                        context_menu, loop.QuitClosure()));
        }));
    ActiveWebContents()
        ->GetPrimaryMainFrame()
        ->GetRenderViewHost()
        ->GetWidget()
        ->ShowContextMenuAtPoint(gfx::Point(x, y),
                                 ui::mojom::MenuSourceType::kMouse);
    loop.Run();
  }

 private:
  base::test::ScopedFeatureList feature_list_{
      email_aliases::features::kEmailAliases};
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::EmbeddedTestServer https_server_{net::EmbeddedTestServer::TYPE_HTTPS};
};

IN_PROC_BROWSER_TEST_F(EmailAliasesBrowserTest, ContextMenuNotAuthorized) {
  const GURL settings_page("chrome://settings/email-aliases");

  Navigate(GURL("https://a.test/email_aliases/inputs.html"));

  content::TestNavigationObserver waiter(settings_page);
  waiter.StartWatchingNewWebContents();
  RunContextMenuOn("type-email");
  waiter.WaitForNavigationFinished();

  EXPECT_EQ(ActiveWebContents()->GetLastCommittedURL(), settings_page);
}

IN_PROC_BROWSER_TEST_F(EmailAliasesBrowserTest, ContextMenuAuthorized) {
  email_aliases_service()->RequestAuthentication("success@domain.com",
                                                 base::DoNothing());
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return !email_aliases_service()->GetAuthTokenForTesting().empty();
  }));

  Navigate(GURL("https://a.test/email_aliases/inputs.html"));

  auto* email_aliases_controller =
      browser()->GetFeatures().email_aliases_controller();
  RunContextMenuOn("type-email");
  // Wait for bubble.
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !!email_aliases_controller->GetBubbleForTesting(); }));
}

}  // namespace email_aliases
