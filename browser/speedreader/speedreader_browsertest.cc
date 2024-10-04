/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string_view>

#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/escape.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/speedreader/page_distiller.h"
#include "brave/browser/speedreader/speedreader_service_factory.h"
#include "brave/browser/speedreader/speedreader_tab_helper.h"
#include "brave/browser/ui/page_action/brave_page_action_icon_type.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/webui/speedreader/speedreader_toolbar_data_handler_impl.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/speedreader/common/constants.h"
#include "brave/components/speedreader/common/features.h"
#include "brave/components/speedreader/common/speedreader.mojom.h"
#include "brave/components/speedreader/common/speedreader_toolbar.mojom.h"
#include "brave/components/speedreader/speedreader_service.h"
#include "brave/components/speedreader/speedreader_util.h"
#include "chrome/browser/profiles/keep_alive/profile_keep_alive_types.h"
#include "chrome/browser/profiles/keep_alive/scoped_profile_keep_alive.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/dom_distiller/core/dom_distiller_switches.h"
#include "components/keep_alive_registry/keep_alive_types.h"
#include "components/keep_alive_registry/scoped_keep_alive.h"
#include "components/language/core/browser/language_prefs.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/browser/reload_type.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "services/network/public/cpp/network_switches.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/common/features.h"
#endif

constexpr char kTestHost[] = "a.test";
constexpr char kTestPageSimple[] = "/simple.html";
constexpr char kTestPageReadable[] = "/speedreader/article/guardian.html";
constexpr char kTestEsPageReadable[] = "/speedreader/article/es.html";
constexpr char kTestPageReadableOnUnreadablePath[] =
    "/speedreader/pages/simple.html";
constexpr char kTestPageRedirect[] = "/articles/redirect_me.html";
constexpr char kTestXml[] = "/speedreader/article/rss.xml";
constexpr char kTestTtsSimple[] = "/speedreader/article/simple.html";
constexpr char kTestTtsTags[] = "/speedreader/article/tags.html";
constexpr char kTestTtsStructure[] = "/speedreader/article/structure.html";
constexpr char kTestErrorPage[] =
    "/speedreader/article/page_not_reachable.html";
constexpr char kTestCSPHtmlPage[] = "/speedreader/article/csp_html.html";
constexpr char kTestCSPHttpPage[] = "/speedreader/article/csp_http.html";
constexpr char kTestCSPHackEquivPage[] =
    "/speedreader/article/csp_hack_equiv.html";
constexpr char kTestCSPHackCharsetPage[] =
    "/speedreader/article/csp_hack_charset.html";
constexpr char kTestCSPOrderPage1[] = "/speedreader/article/csp_order_1.html";
constexpr char kTestCSPOrderPage2[] = "/speedreader/article/csp_order_2.html";
constexpr char kTestCSPInBodyPage[] = "/speedreader/article/csp_in_body.html";

class SpeedReaderBrowserTest : public InProcessBrowserTest {
 public:
  SpeedReaderBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
#if BUILDFLAG(ENABLE_AI_CHAT)
    feature_list_.InitWithFeaturesAndParameters(
        {{speedreader::kSpeedreaderFeature,
          {
            { speedreader::kSpeedreaderTTS.name,
              "true" }
          }},
         { ai_chat::features::kAIChat,
           { {} } }},
        {});
#else
    feature_list_.InitAndEnableFeatureWithParameters(
        speedreader::kSpeedreaderFeature,
        {{speedreader::kSpeedreaderTTS.name, "true"}});
#endif
  }

  SpeedReaderBrowserTest(const SpeedReaderBrowserTest&) = delete;
  SpeedReaderBrowserTest& operator=(const SpeedReaderBrowserTest&) = delete;

  ~SpeedReaderBrowserTest() override = default;

  void SetUp() override {
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    ASSERT_TRUE(https_server_.InitializeAndListen());
    InProcessBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    auto redirector = [](const net::test_server::HttpRequest& request)
        -> std::unique_ptr<net::test_server::HttpResponse> {
      if (request.GetURL().path_piece() != kTestPageRedirect) {
        return nullptr;
      }
      const std::string dest =
          base::UnescapeBinaryURLComponent(request.GetURL().query_piece());

      auto http_response =
          std::make_unique<net::test_server::BasicHttpResponse>();
      http_response->set_code(net::HTTP_MOVED_PERMANENTLY);
      http_response->AddCustomHeader("Location", dest);
      http_response->AddCustomHeader("Access-Control-Allow-Origin", "*");
      http_response->set_content_type("text/html");
      return http_response;
    };

    https_server_.RegisterDefaultHandler(base::BindRepeating(redirector));
    https_server_.ServeFilesFromDirectory(
        base::PathService::CheckedGet(brave::DIR_TEST_DATA));
    https_server_.StartAcceptingConnections();
    host_resolver()->AddRule("*", "127.0.0.1");
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(
        network::switches::kHostResolverRules,
        "MAP *:443 " + https_server_.host_port_pair().ToString());
  }

  content::WebContents* ActiveWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  speedreader::SpeedreaderTabHelper* tab_helper() {
    return speedreader::SpeedreaderTabHelper::FromWebContents(
        ActiveWebContents());
  }

  speedreader::SpeedreaderService* speedreader_service() {
    return speedreader::SpeedreaderServiceFactory::GetForBrowserContext(
        browser()->profile());
  }

  void NonBlockingDelay(const base::TimeDelta& delay) {
    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE, run_loop.QuitWhenIdleClosure(), delay);
    run_loop.Run();
  }

  PageActionIconView* GetReaderButton() {
    return BrowserView::GetBrowserViewForBrowser(browser())
        ->toolbar_button_provider()
        ->GetPageActionIconView(brave::kSpeedreaderPageActionIconType);
  }

  void WaitDistilled(speedreader::SpeedreaderTabHelper* th = nullptr) {
    if (!th) {
      th = tab_helper();
    }
    while (!speedreader::DistillStates::IsDistilled(th->PageDistillState())) {
      NonBlockingDelay(base::Milliseconds(10));
    }
    content::WaitForLoadStop(ActiveWebContents());
  }

  void WaitDistillable(speedreader::SpeedreaderTabHelper* th = nullptr) {
    if (!th) {
      th = tab_helper();
    }
    while (!speedreader::DistillStates::IsDistillable(th->PageDistillState())) {
      NonBlockingDelay(base::Milliseconds(10));
    }
    content::WaitForLoadStop(ActiveWebContents());
  }

  void WaitOriginal(speedreader::SpeedreaderTabHelper* th = nullptr) {
    if (!th) {
      th = tab_helper();
    }
    while (
        !speedreader::DistillStates::IsViewOriginal(th->PageDistillState())) {
      NonBlockingDelay(base::Milliseconds(10));
    }
    content::WaitForLoadStop(ActiveWebContents());
  }

  void ClickReaderButton() {
    const auto was_distilled = speedreader::DistillStates::IsDistilled(
        tab_helper()->PageDistillState());
    browser()->command_controller()->ExecuteCommand(
        IDC_SPEEDREADER_ICON_ONCLICK);
    if (!was_distilled) {
      WaitDistilled();
    } else {
      WaitDistillable();
    }
    content::WaitForLoadStop(ActiveWebContents());
  }

  void ToggleSpeedreader() {
    speedreader_service()->EnableForAllSites(
        !speedreader_service()->IsEnabledForAllSites());
  }

  void DisableSpeedreader() { speedreader_service()->EnableForAllSites(false); }

  void GoBack(Browser* browser) {
    content::TestNavigationObserver observer(ActiveWebContents());
    chrome::GoBack(browser, WindowOpenDisposition::CURRENT_TAB);
    observer.Wait();
  }

  void NavigateToPageSynchronously(
      std::string_view path,
      WindowOpenDisposition disposition =
          WindowOpenDisposition::NEW_FOREGROUND_TAB) {
    const GURL url = GURL("https://a.test").Resolve(path);
    ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
        browser(), url, disposition,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
  net::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, PRE_RestoreSpeedreaderPage) {
  ToggleSpeedreader();
  NavigateToPageSynchronously(kTestPageReadable,
                              WindowOpenDisposition::CURRENT_TAB);
  EXPECT_TRUE(speedreader::DistillStates::IsDistilled(
      tab_helper()->PageDistillState()));
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, RestoreSpeedreaderPage) {
  browser()->tab_strip_model()->ActivateTabAt(0);
  WaitDistilled();
  EXPECT_TRUE(speedreader::DistillStates::IsDistilled(
      tab_helper()->PageDistillState()));
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, NavigationNostickTest) {
  ToggleSpeedreader();
  NavigateToPageSynchronously(kTestPageSimple);
  EXPECT_FALSE(speedreader::DistillStates::IsDistilled(
      tab_helper()->PageDistillState()));
  NavigateToPageSynchronously(kTestPageReadable,
                              WindowOpenDisposition::CURRENT_TAB);
  EXPECT_TRUE(speedreader::DistillStates::IsDistilled(
      tab_helper()->PageDistillState()));

  // Ensure distill state doesn't stick when we back-navigate from a readable
  // page to a non-readable one.
  GoBack(browser());
  EXPECT_FALSE(speedreader::DistillStates::IsDistilled(
      tab_helper()->PageDistillState()));
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, DisableSiteWorks) {
  ToggleSpeedreader();
  NavigateToPageSynchronously(kTestPageReadable);
  EXPECT_TRUE(speedreader::DistillStates::IsDistilled(
      tab_helper()->PageDistillState()));
  speedreader_service()->EnableForSite(ActiveWebContents(), false);
  EXPECT_TRUE(WaitForLoadStop(ActiveWebContents()));
  EXPECT_FALSE(speedreader::DistillStates::IsDistilled(
      tab_helper()->PageDistillState()));
}

// I assume that the periodic fails of this test are related to issues/36355, I
// need to deal with it before turning it back. Other tests cover the
// scenario in this one, so a temporary disabling will not affect the health
// check of the feature.
IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, DISABLED_SmokeTest) {
  // Solana web3.js console warning will interfere with console observer
  brave_wallet::SetDefaultSolanaWallet(
      browser()->profile()->GetPrefs(),
      brave_wallet::mojom::DefaultWallet::None);

  const std::string kGetContentLength = "document.body.innerHTML.length";

  // Check that disabled speedreader doesn't affect the page.
  EXPECT_FALSE(speedreader_service()->IsEnabledForAllSites());
  NavigateToPageSynchronously(kTestPageReadable,
                              WindowOpenDisposition::CURRENT_TAB);
  const auto first_load_page_length =
      content::EvalJs(ActiveWebContents(), kGetContentLength,
                      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                      ISOLATED_WORLD_ID_BRAVE_INTERNAL)
          .ExtractInt();
  EXPECT_LT(83000, first_load_page_length);

  ToggleSpeedreader();
  EXPECT_TRUE(speedreader_service()->IsEnabledForAllSites());

  content::WebContentsConsoleObserver console_observer(ActiveWebContents());
  console_observer.SetFilter(base::BindLambdaForTesting(
      [](const content::WebContentsConsoleObserver::Message& message) {
        return message.log_level == blink::mojom::ConsoleMessageLevel::kError;
      }));
  NavigateToPageSynchronously(kTestPageReadable,
                              WindowOpenDisposition::CURRENT_TAB);

  const std::string kGetStyleLength =
      "document.getElementById('brave_speedreader_style').innerHTML.length";
  const std::string kGetFontsExists =
      "!!(document.getElementById('atkinson_hyperligible_font') && "
      "document.getElementById('open_dyslexic_font'))";
  const std::string kCheckReferrer =
      R"js(document.querySelector('meta[name="referrer"]')
             .getAttribute('content') === 'no-referrer')js";
  const std::string kCheckResources =
      "JSON.stringify(speedreaderData) == '{\"minutesText\":\"min. "
      "read\",\"playButtonTitle\":\"Play/"
      "Pause\",\"showOriginalLinkText\":\"View "
      "original\",\"ttsEnabled\":true}'";

  // Check that the document became much smaller and that non-empty
  // speedreader style is injected.
  EXPECT_LT(0, content::EvalJs(ActiveWebContents(), kGetStyleLength,
                               content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                               ISOLATED_WORLD_ID_BRAVE_INTERNAL)
                   .ExtractInt());
  EXPECT_TRUE(content::EvalJs(ActiveWebContents(), kGetFontsExists,
                              content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                              ISOLATED_WORLD_ID_BRAVE_INTERNAL)
                  .ExtractBool());
  EXPECT_TRUE(content::EvalJs(ActiveWebContents(), kCheckReferrer,
                              content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                              ISOLATED_WORLD_ID_BRAVE_INTERNAL)
                  .ExtractBool());
  EXPECT_TRUE(content::EvalJs(ActiveWebContents(), kCheckResources,
                              content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                              ISOLATED_WORLD_ID_BRAVE_INTERNAL)
                  .ExtractBool());

  const auto speedreaded_length =
      content::EvalJs(ActiveWebContents(), kGetContentLength,
                      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                      ISOLATED_WORLD_ID_BRAVE_INTERNAL)
          .ExtractInt();
  EXPECT_GT(17750, speedreaded_length);

  EXPECT_TRUE(console_observer.messages().empty());

  ToggleSpeedreader();
  EXPECT_FALSE(speedreader_service()->IsEnabledForAllSites());

  NavigateToPageSynchronously(kTestPageReadable);
  auto second_load_page_length =
      content::EvalJs(ActiveWebContents(), kGetContentLength,
                      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                      ISOLATED_WORLD_ID_BRAVE_INTERNAL)
          .ExtractInt();
  if (second_load_page_length == 1) {
    // TODO(issues/36355): Sometimes browser failed to load this page.
    ActiveWebContents()->GetController().Reload(content::ReloadType::NORMAL,
                                                false);
    content::WaitForLoadStop(ActiveWebContents());
    second_load_page_length =
        content::EvalJs(ActiveWebContents(), kGetContentLength,
                        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                        ISOLATED_WORLD_ID_BRAVE_INTERNAL)
            .ExtractInt();
  }

  EXPECT_LT(83000, second_load_page_length)
      << " First load length: " << first_load_page_length
      << " speedreaded length: " << speedreaded_length
      << " Second load length: " << second_load_page_length;
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, Redirect) {
  ToggleSpeedreader();

  const auto redirect_url = https_server_.GetURL(
      kTestHost, "/speedreader/rewriter/jsonld_shortest_desc.html");
  NavigateToPageSynchronously(kTestPageRedirect + ("?" + redirect_url.spec()));

  const std::string kCheckNoStyle =
      "!document.getElementById('brave_speedreader_style')";

  EXPECT_TRUE(content::EvalJs(ActiveWebContents(), kCheckNoStyle,
                              content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                              ISOLATED_WORLD_ID_BRAVE_INTERNAL)
                  .ExtractBool());
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, ClickingOnReaderButton) {
  EXPECT_FALSE(speedreader_service()->IsEnabledForAllSites());

  NavigateToPageSynchronously(kTestPageReadable);
  EXPECT_TRUE(GetReaderButton()->GetVisible());

  EXPECT_FALSE(speedreader::DistillStates::IsDistilled(
      tab_helper()->PageDistillState()));
  ClickReaderButton();
  EXPECT_TRUE(GetReaderButton()->GetVisible());
  EXPECT_TRUE(speedreader::DistillStates::IsDistilled(
      tab_helper()->PageDistillState()));
  EXPECT_TRUE(GetReaderButton()->GetVisible());

  ClickReaderButton();
  EXPECT_TRUE(GetReaderButton()->GetVisible());
  EXPECT_TRUE(speedreader::DistillStates::IsViewOriginal(
      tab_helper()->PageDistillState()));

  EXPECT_FALSE(speedreader_service()->IsEnabledForAllSites());
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, OnDemandReader) {
  EXPECT_FALSE(speedreader_service()->IsEnabledForAllSites());

  NavigateToPageSynchronously(kTestPageReadable);
  EXPECT_TRUE(GetReaderButton()->GetVisible());

  EXPECT_TRUE(speedreader::DistillStates::IsDistillable(
      tab_helper()->PageDistillState()));
  // Change content on the page.
  constexpr const char kChangeContent[] =
      R"js(
        document.querySelector('meta[property="og:title"]').content =
            'Title was changed by javascript'
      )js";
  EXPECT_TRUE(content::ExecJs(ActiveWebContents(), kChangeContent,
                              content::EXECUTE_SCRIPT_DEFAULT_OPTIONS));
  ClickReaderButton();

  EXPECT_TRUE(speedreader::DistillStates::IsDistilled(
      tab_helper()->PageDistillState()));

  // Check title on the distilled page.
  constexpr const char kCheckContent[] =
      R"js(
        !!document.getElementById('brave_speedreader_style') &&
        (document.title === 'Title was changed by javascript')
      )js";
  EXPECT_TRUE(content::EvalJs(ActiveWebContents(), kCheckContent,
                              content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                              ISOLATED_WORLD_ID_BRAVE_INTERNAL)
                  .ExtractBool());
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, OnDemandReaderEncoding) {
  EXPECT_FALSE(speedreader_service()->IsEnabledForAllSites());
  NavigateToPageSynchronously(kTestEsPageReadable);
  EXPECT_TRUE(GetReaderButton()->GetVisible());
  ClickReaderButton();

  constexpr const char kCheckText[] =
      R"js( document.querySelector('#par-to-check').innerText.length )js";
  EXPECT_EQ(92, content::EvalJs(ActiveWebContents(), kCheckText,
                                content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                                ISOLATED_WORLD_ID_BRAVE_INTERNAL)
                    .ExtractInt());
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, EnableDisableSpeedreaderA) {
  EXPECT_FALSE(speedreader_service()->IsEnabledForAllSites());
  NavigateToPageSynchronously(kTestPageReadable);

  EXPECT_TRUE(GetReaderButton()->GetVisible());
  EXPECT_TRUE(speedreader::DistillStates::IsDistillable(
      tab_helper()->PageDistillState()));
  ToggleSpeedreader();
  WaitDistilled();
  EXPECT_TRUE(GetReaderButton()->GetVisible());
  EXPECT_TRUE(speedreader::DistillStates::IsDistilled(
      tab_helper()->PageDistillState()));
  DisableSpeedreader();
  WaitOriginal();
  EXPECT_TRUE(GetReaderButton()->GetVisible());
  EXPECT_TRUE(speedreader::DistillStates::IsDistillable(
      tab_helper()->PageDistillState()));
  EXPECT_TRUE(speedreader::DistillStates::IsViewOriginal(
      tab_helper()->PageDistillState()));
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, EnableDisableSpeedreaderB) {
  NavigateToPageSynchronously(kTestPageReadable);
  ClickReaderButton();
  WaitDistilled();
  EXPECT_TRUE(GetReaderButton()->GetVisible());
  EXPECT_TRUE(speedreader::DistillStates::IsDistilled(
      tab_helper()->PageDistillState()));
  ToggleSpeedreader();
  WaitDistilled();
  EXPECT_TRUE(GetReaderButton()->GetVisible());
  EXPECT_TRUE(speedreader::DistillStates::IsDistilled(
      tab_helper()->PageDistillState()));
  DisableSpeedreader();
  WaitOriginal();
  EXPECT_TRUE(GetReaderButton()->GetVisible());
  EXPECT_TRUE(speedreader::DistillStates::IsDistillable(
      tab_helper()->PageDistillState()));
  EXPECT_TRUE(speedreader::DistillStates::IsViewOriginal(
      tab_helper()->PageDistillState()));
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, TogglingSiteSpeedreader) {
  ToggleSpeedreader();
  NavigateToPageSynchronously(kTestPageReadable);

  for (int i = 0; i < 2; ++i) {
    EXPECT_TRUE(WaitForLoadStop(ActiveWebContents()));
    EXPECT_TRUE(speedreader::DistillStates::IsDistilled(
        tab_helper()->PageDistillState()));
    EXPECT_TRUE(GetReaderButton()->GetVisible());

    speedreader_service()->EnableForSite(ActiveWebContents(), false);
    EXPECT_TRUE(WaitForLoadStop(ActiveWebContents()));
    EXPECT_TRUE(speedreader::DistillStates::IsViewOriginal(
        tab_helper()->PageDistillState()));
    EXPECT_TRUE(GetReaderButton()->GetVisible());

    speedreader_service()->EnableForSite(ActiveWebContents(), true);
    EXPECT_TRUE(WaitForLoadStop(ActiveWebContents()));
  }
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, ReloadContent) {
  ToggleSpeedreader();
  NavigateToPageSynchronously(kTestPageReadable);
  auto* contents_1 = ActiveWebContents();
  NavigateToPageSynchronously(kTestPageReadable);
  auto* contents_2 = ActiveWebContents();

  auto* tab_helper_1 =
      speedreader::SpeedreaderTabHelper::FromWebContents(contents_1);
  auto* tab_helper_2 =
      speedreader::SpeedreaderTabHelper::FromWebContents(contents_2);

  EXPECT_TRUE(speedreader::DistillStates::IsDistilled(
      tab_helper_1->PageDistillState()));
  EXPECT_TRUE(speedreader::DistillStates::IsDistilled(
      tab_helper_2->PageDistillState()));

  speedreader_service()->EnableForSite(tab_helper_1->web_contents(), false);
  content::WaitForLoadStop(contents_1);
  EXPECT_TRUE(speedreader::DistillStates::IsViewOriginal(
      tab_helper_1->PageDistillState()));
  EXPECT_TRUE(speedreader::DistillStates::IsDistilled(
      tab_helper_2->PageDistillState()));

  contents_2->GetController().Reload(content::ReloadType::NORMAL, false);
  content::WaitForLoadStop(contents_2);

  EXPECT_TRUE(speedreader::DistillStates::IsViewOriginal(
      tab_helper_1->PageDistillState()));
  EXPECT_TRUE(speedreader::DistillStates::IsViewOriginal(
      tab_helper_2->PageDistillState()));
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, ShowOriginalPage) {
  ToggleSpeedreader();
  NavigateToPageSynchronously(kTestPageReadable);
  auto* web_contents = ActiveWebContents();

  constexpr const char kCheckNoApiInMainWorld[] =
      R"js(
        document.speedreader === undefined
      )js";
  EXPECT_TRUE(content::EvalJs(web_contents, kCheckNoApiInMainWorld,
                              content::EXECUTE_SCRIPT_DEFAULT_OPTIONS)
                  .ExtractBool());

  constexpr const char kClickLinkAndGetTitle[] =
      R"js(
    (function() {
      // element id is hardcoded in extractor.rs
      const link =
        document.getElementById('c93e2206-2f31-4ddc-9828-2bb8e8ed940e');
      link.click();
      return link.innerText
    })();
  )js";

  EXPECT_EQ("View original",
            content::EvalJs(web_contents, kClickLinkAndGetTitle,
                            content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                            ISOLATED_WORLD_ID_BRAVE_INTERNAL)
                .ExtractString());
  content::WaitForLoadStop(web_contents);
  auto* tab_helper =
      speedreader::SpeedreaderTabHelper::FromWebContents(web_contents);
  EXPECT_TRUE(speedreader::DistillStates::IsDistillable(
      tab_helper->PageDistillState()));
  EXPECT_TRUE(speedreader_service()->IsEnabledForSite(web_contents));

  // Click on speedreader button
  ClickReaderButton();
  content::WaitForLoadStop(web_contents);
  EXPECT_TRUE(
      speedreader::DistillStates::IsDistilled(tab_helper->PageDistillState()));
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, ShowOriginalPageOnUnreadable) {
  ToggleSpeedreader();
  NavigateToPageSynchronously(kTestPageSimple);
  auto* web_contents = ActiveWebContents();

  constexpr const char kCheckNoElement[] =
      R"js(
        document.getElementById('c93e2206-2f31-4ddc-9828-2bb8e8ed940e') == null
      )js";

  EXPECT_TRUE(content::EvalJs(web_contents, kCheckNoElement,
                              content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                              ISOLATED_WORLD_ID_BRAVE_INTERNAL)
                  .ExtractBool());

  constexpr const char kCheckNoApi[] =
      R"js(
        document.speedreader === undefined
      )js";

  EXPECT_TRUE(content::EvalJs(web_contents, kCheckNoApi,
                              content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                              ISOLATED_WORLD_ID_BRAVE_INTERNAL)
                  .ExtractBool());
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, SetDataAttributes) {
  ToggleSpeedreader();
  NavigateToPageSynchronously(kTestPageReadable);
  auto* contents = ActiveWebContents();

  // Open second tab
  NavigateToPageSynchronously(kTestPageReadable);

  auto GetDataAttribute = [](const std::string& attr) {
    constexpr const char kGetDataAttribute[] =
        R"js(
          document.documentElement.getAttribute('$1')
        )js";
    return base::ReplaceStringPlaceholders(kGetDataAttribute, {attr}, nullptr);
  };

  EXPECT_EQ(speedreader::mojom::Theme::kNone,
            speedreader_service()->GetAppearanceSettings().theme);
  EXPECT_EQ(speedreader::mojom::FontFamily::kSans,
            speedreader_service()->GetAppearanceSettings().fontFamily);
  EXPECT_EQ(speedreader::mojom::FontSize::k100,
            speedreader_service()->GetAppearanceSettings().fontSize);
  EXPECT_EQ(speedreader::mojom::ColumnWidth::kNarrow,
            speedreader_service()->GetAppearanceSettings().columnWidth);

  EXPECT_EQ(nullptr, content::EvalJs(contents, GetDataAttribute("data-theme"),
                                     content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                                     ISOLATED_WORLD_ID_BRAVE_INTERNAL));
  speedreader_service()->SetAppearanceSettings(
      speedreader::mojom::AppearanceSettings(
          speedreader::mojom::Theme::kDark, speedreader::mojom::FontSize::k130,
          speedreader::mojom::FontFamily::kDyslexic,
          speedreader::mojom::ColumnWidth::kWide));

  auto EvalAttr = [&](content::WebContents* contents, const std::string& attr) {
    return content::EvalJs(contents, GetDataAttribute(attr),
                           content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                           ISOLATED_WORLD_ID_BRAVE_INTERNAL)
        .ExtractString();
  };

  EXPECT_EQ("dark", EvalAttr(contents, "data-theme"));
  EXPECT_EQ("dyslexic", EvalAttr(contents, "data-font-family"));
  EXPECT_EQ("130", EvalAttr(contents, "data-font-size"));
  EXPECT_EQ("wide", EvalAttr(contents, "data-column-width"));

  // Same in the second tab
  EXPECT_EQ("dark", EvalAttr(ActiveWebContents(), "data-theme"));
  EXPECT_EQ("dyslexic", EvalAttr(ActiveWebContents(), "data-font-family"));
  EXPECT_EQ("130", EvalAttr(ActiveWebContents(), "data-font-size"));
  EXPECT_EQ("wide", EvalAttr(contents, "data-column-width"));

  EXPECT_EQ(speedreader::mojom::Theme::kDark,
            speedreader_service()->GetAppearanceSettings().theme);
  EXPECT_EQ(speedreader::mojom::FontFamily::kDyslexic,
            speedreader_service()->GetAppearanceSettings().fontFamily);
  EXPECT_EQ(speedreader::mojom::FontSize::k130,
            speedreader_service()->GetAppearanceSettings().fontSize);
  EXPECT_EQ(speedreader::mojom::ColumnWidth::kWide,
            speedreader_service()->GetAppearanceSettings().columnWidth);

  // New page
  NavigateToPageSynchronously(kTestPageReadable);
  EXPECT_EQ("dark", EvalAttr(ActiveWebContents(), "data-theme"));
  EXPECT_EQ("dyslexic", EvalAttr(ActiveWebContents(), "data-font-family"));
  EXPECT_EQ("130", EvalAttr(ActiveWebContents(), "data-font-size"));
  EXPECT_EQ("wide", EvalAttr(contents, "data-column-width"));
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, Toolbar) {
  auto GetDataAttribute = [](const std::string& attr) {
    constexpr const char kGetDataAttribute[] =
        R"js(
          document.documentElement.getAttribute('$1')
        )js";
    return base::ReplaceStringPlaceholders(kGetDataAttribute, {attr}, nullptr);
  };

  auto WaitAttr = [&](content::WebContents* contents, const std::string& attr,
                      const std::string& value) {
    for (;;) {
      NonBlockingDelay(base::Milliseconds(10));
      auto eval = content::EvalJs(contents, GetDataAttribute(attr),
                                  content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                                  ISOLATED_WORLD_ID_BRAVE_INTERNAL);
      if (!eval.value.is_string() && value.empty()) {
        return true;
      }
      if (eval.ExtractString() == value) {
        return true;
      }
    }
  };

  auto WaitElement = [&](content::WebContents* contents,
                         const std::string& elem) {
    constexpr const char kWaitElement[] =
        R"js(
          (!!document.getElementById('$1'))
        )js";
    for (;;) {
      NonBlockingDelay(base::Milliseconds(10));
      if (content::EvalJs(
              contents,
              base::ReplaceStringPlaceholders(kWaitElement, {elem}, nullptr),
              content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
              ISOLATED_WORLD_ID_BRAVE_INTERNAL)
              .ExtractBool()) {
        break;
      }
    }
  };

  auto Click = [&](content::WebContents* contents, const std::string& id) {
    constexpr const char kClick[] =
        R"js(
          document.getElementById('$1').click()
        )js";
    ASSERT_TRUE(content::ExecJs(
        contents, base::ReplaceStringPlaceholders(kClick, {id}, nullptr)));
  };

  ToggleSpeedreader();
  NavigateToPageSynchronously(kTestPageReadable);

  auto* page = ActiveWebContents();
  auto* toolbar_view = static_cast<BraveBrowserView*>(browser()->window())
                           ->reader_mode_toolbar_view_.get();
  auto* toolbar = toolbar_view->GetWebContentsForTesting();
  WaitElement(toolbar, "appearance");

#if BUILDFLAG(ENABLE_AI_CHAT)
  Click(toolbar, "ai");
  auto* side_panel = browser()->GetFeatures().side_panel_ui();
  while (side_panel->GetCurrentEntryId() != SidePanelEntryId::kChatUI) {
    NonBlockingDelay(base::Milliseconds(10));
  }
  EXPECT_EQ(SidePanelEntryId::kChatUI, side_panel->GetCurrentEntryId());
  Click(toolbar, "ai");
  while (side_panel->GetCurrentEntryId().has_value()) {
    NonBlockingDelay(base::Milliseconds(10));
  }
  EXPECT_FALSE(side_panel->GetCurrentEntryId().has_value());
#endif

  Click(toolbar, "appearance");
  {  // change theme
    Click(toolbar, "theme-light");
    WaitAttr(page, "data-theme", "light");
    Click(toolbar, "theme-sepia");
    WaitAttr(page, "data-theme", "sepia");
    Click(toolbar, "theme-dark");
    WaitAttr(page, "data-theme", "dark");
    Click(toolbar, "theme-system");
    WaitAttr(page, "data-theme", "");
  }
  {  // change font
    Click(toolbar, "font-sans");
    WaitAttr(page, "data-font-family", "sans");
    Click(toolbar, "font-serif");
    WaitAttr(page, "data-font-family", "serif");
    Click(toolbar, "font-mono");
    WaitAttr(page, "data-font-family", "mono");
    Click(toolbar, "font-dyslexic");
    WaitAttr(page, "data-font-family", "dyslexic");
  }
  {  // change font size
    WaitAttr(page, "data-font-size", "100");
    Click(toolbar, "font-size-decrease");
    WaitAttr(page, "data-font-size", "90");
    Click(toolbar, "font-size-increase");
    WaitAttr(page, "data-font-size", "100");
    Click(toolbar, "font-size-increase");
    WaitAttr(page, "data-font-size", "110");
  }
  Click(toolbar, "appearance");

  Click(toolbar, "tune");
  {
    while (!tab_helper()->speedreader_bubble_view()) {
      NonBlockingDelay(base::Milliseconds(10));
    }
  }
  Click(toolbar, "tune");

  Click(toolbar, "close");
  {
    WaitOriginal();
    EXPECT_FALSE(toolbar_view->GetVisible());
  }
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, ToolbarLangs) {
  language::LanguagePrefs language_prefs(browser()->profile()->GetPrefs());
  language_prefs.SetUserSelectedLanguagesList(
      {"en-US", "ja", "en-CA", "fr-CA"});

  ToggleSpeedreader();
  NavigateToPageSynchronously(kTestPageReadable);

  auto* toolbar_view = static_cast<BraveBrowserView*>(browser()->window())
                           ->reader_mode_toolbar_view_.get();
  auto* toolbar = toolbar_view->GetWebContentsForTesting();

  constexpr const char kGetLang[] = R"js( navigator.languages.toString() )js";
  EXPECT_EQ("en-US,ja,en-CA,fr-CA",
            content::EvalJs(toolbar, kGetLang).ExtractString());
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, RSS) {
  ToggleSpeedreader();
  NavigateToPageSynchronously(kTestXml);

  EXPECT_FALSE(GetReaderButton()->GetVisible());

  const std::string kNoStyleInjected =
      R"js(document.getElementById('brave_speedreader_style'))js";

  EXPECT_EQ(nullptr, content::EvalJs(ActiveWebContents(), kNoStyleInjected,
                                     content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                                     ISOLATED_WORLD_ID_BRAVE_INTERNAL));
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, TTS) {
  ToggleSpeedreader();

  const std::string kCheckTtsParagraphs = R"js(
    document.querySelectorAll('[tts-paragraph-index]').length
  )js";

  const char* pages[] = {kTestTtsSimple, kTestTtsTags, kTestTtsStructure};
  for (const auto* page : pages) {
    NavigateToPageSynchronously(page);
    SCOPED_TRACE(page);
    EXPECT_EQ(7, content::EvalJs(ActiveWebContents(), kCheckTtsParagraphs,
                                 content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                                 ISOLATED_WORLD_ID_BRAVE_INTERNAL)
                     .ExtractInt());
  }
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, ErrorPage) {
  ToggleSpeedreader();
  NavigateToPageSynchronously(kTestErrorPage,
                              WindowOpenDisposition::CURRENT_TAB);
  EXPECT_TRUE(ActiveWebContents()->GetPrimaryMainFrame()->IsErrorDocument());
  EXPECT_FALSE(GetReaderButton()->GetVisible());

  // Navigate to the non-automatic distillable page.
  NavigateToPageSynchronously(kTestPageReadableOnUnreadablePath,
                              WindowOpenDisposition::CURRENT_TAB);
  EXPECT_TRUE(speedreader::DistillStates::IsViewOriginal(
      tab_helper()->PageDistillState()));
  WaitDistillable(tab_helper());
  EXPECT_TRUE(GetReaderButton()->GetVisible());

  GoBack(browser());
  NavigateToPageSynchronously(kTestPageReadable,
                              WindowOpenDisposition::CURRENT_TAB);
  WaitDistilled();
  EXPECT_TRUE(GetReaderButton()->GetVisible());
  EXPECT_TRUE(speedreader::DistillStates::IsDistilled(
      tab_helper()->PageDistillState()));
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, Csp) {
  ToggleSpeedreader();

  for (const auto* page : {kTestCSPHackEquivPage, kTestCSPHackCharsetPage,
                           kTestCSPHtmlPage, kTestCSPHttpPage}) {
    SCOPED_TRACE(page);

    content::WebContentsConsoleObserver console_observer(ActiveWebContents());
    console_observer.SetPattern(
        "Refused to load the image 'https://a.test/should_fail.png' because it "
        "violates the following Content Security Policy directive: \"img-src "
        "'none'\".*");

    NavigateToPageSynchronously(page, WindowOpenDisposition::CURRENT_TAB);

    constexpr const char kCheckNoMaliciousContent[] = R"js(
      !document.getElementById('malicious1') &&
      !document.querySelector('meta[http-equiv="undefinedHttpEquiv"]')
    )js";
    EXPECT_EQ(true,
              content::EvalJs(ActiveWebContents(), kCheckNoMaliciousContent,
                              content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                              ISOLATED_WORLD_ID_BRAVE_INTERNAL));

    EXPECT_TRUE(console_observer.Wait());
  }
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, CspOrder) {
  ToggleSpeedreader();

  // base first.
  {
    content::WebContentsConsoleObserver console_observer(ActiveWebContents());
    NavigateToPageSynchronously(kTestCSPOrderPage1,
                                WindowOpenDisposition::CURRENT_TAB);
    EXPECT_TRUE(console_observer.messages().empty());
  }

  // CSP first.
  {
    content::WebContentsConsoleObserver console_observer(ActiveWebContents());
    console_observer.SetPattern(
        "Refused to set the document's base URI to 'https://a.test/' because "
        "it violates the following Content Security Policy directive: "
        "\"base-uri 'none'\".*");
    NavigateToPageSynchronously(kTestCSPOrderPage2,
                                WindowOpenDisposition::CURRENT_TAB);
    EXPECT_TRUE(console_observer.Wait());
  }
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, CspInBody) {
  ToggleSpeedreader();

  NavigateToPageSynchronously(kTestCSPInBodyPage,
                              WindowOpenDisposition::CURRENT_TAB);
  constexpr const char kCheckCsp[] = R"js(
    document.querySelectorAll('meta[content="CSP in body"]').length === 0
  )js";

  EXPECT_EQ(true, content::EvalJs(ActiveWebContents(), kCheckCsp,
                                  content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                                  ISOLATED_WORLD_ID_BRAVE_INTERNAL));
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, OnDemandReaderEnableForSite) {
  EXPECT_FALSE(speedreader_service()->IsEnabledForAllSites());

  struct MockObserver : speedreader::PageDistiller::Observer {
    MOCK_METHOD(void,
                OnPageDistillStateChanged,
                (speedreader::PageDistiller::State),
                (override));
  };

  testing::NiceMock<MockObserver> observer;
  tab_helper()->speedreader::PageDistiller::AddObserver(&observer);

  base::RunLoop run_loop;
  ON_CALL(observer, OnPageDistillStateChanged(
                        speedreader::PageDistiller::State::kDistillable))
      .WillByDefault(
          testing::InvokeWithoutArgs(&run_loop, &base::RunLoop::Quit));

  NavigateToPageSynchronously(kTestPageReadableOnUnreadablePath,
                              WindowOpenDisposition::CURRENT_TAB);
  run_loop.Run();
  tab_helper()->speedreader::PageDistiller::RemoveObserver(&observer);

  EXPECT_TRUE(GetReaderButton()->GetVisible());

  EXPECT_TRUE(speedreader::DistillStates::IsDistillable(
      tab_helper()->PageDistillState()));

  ClickReaderButton();

  EXPECT_TRUE(speedreader::DistillStates::IsDistilled(
      tab_helper()->PageDistillState()));

  // Enable speedreader for site explicitly.
  speedreader_service()->EnableForSite(ActiveWebContents(), true);
  ActiveWebContents()->GetController().Reload(content::ReloadType::NORMAL,
                                              false);
  WaitDistilled();

  EXPECT_TRUE(speedreader::DistillStates::IsDistilled(
      tab_helper()->PageDistillState()));

  // Go to home page.
  NavigateToPageSynchronously("/", WindowOpenDisposition::CURRENT_TAB);
  EXPECT_TRUE(speedreader::DistillStates::IsViewOriginal(
      tab_helper()->PageDistillState()));
}
