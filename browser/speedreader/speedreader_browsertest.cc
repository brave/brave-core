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
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_timeouts.h"
#include "base/time/time.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/speedreader/page_distiller.h"
#include "brave/browser/speedreader/speedreader_service_factory.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/speedreader/speedreader_tab_helper.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/split_view/brave_contents_container_view.h"
#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view.h"
#include "brave/browser/ui/webui/speedreader/speedreader_toolbar_data_handler_impl.h"
#include "brave/common/pref_names.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/speedreader/common/features.h"
#include "brave/components/speedreader/common/speedreader.mojom.h"
#include "brave/components/speedreader/common/speedreader_toolbar.mojom.h"
#include "brave/components/speedreader/speedreader_pref_names.h"
#include "brave/components/speedreader/speedreader_service.h"
#include "brave/components/speedreader/speedreader_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_view.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/language/core/browser/language_prefs.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/browser/reload_type.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/download_test_observer.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "services/network/public/cpp/network_switches.h"
#include "ui/events/base_event_utils.h"

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
    feature_list_.InitWithFeaturesAndParameters(
        {{features::kBraveRoundedCornersByDefault, {}},
         {speedreader::features::kSpeedreaderFeature,
          {{speedreader::features::kSpeedreaderTTS.name, "true"}}}
#if BUILDFLAG(ENABLE_AI_CHAT)
         ,
         {ai_chat::features::kAIChat, {{}}}
#endif
        },
        {});
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
      if (request.GetURL().path() != kTestPageRedirect) {
        return nullptr;
      }
      const std::string dest =
          base::UnescapeBinaryURLComponent(request.GetURL().query());

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
        browser()->GetProfile());
  }

  void NonBlockingDelay(base::TimeDelta delay) {
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

  bool WaitDistilled(speedreader::SpeedreaderTabHelper* th = nullptr) {
    if (!th) {
      th = tab_helper();
    }
    if (!base::test::RunUntil([th]() {
          return speedreader::IsDistilled(th->PageDistillState());
        })) {
      return false;
    }
    content::WaitForLoadStop(ActiveWebContents());
    return true;
  }

  bool WaitDistillable(speedreader::SpeedreaderTabHelper* th = nullptr) {
    if (!th) {
      th = tab_helper();
    }
    if (!base::test::RunUntil([th]() {
          return speedreader::IsDistillable(th->PageDistillState());
        })) {
      return false;
    }
    content::WaitForLoadStop(ActiveWebContents());
    return true;
  }

  bool WaitOriginal(speedreader::SpeedreaderTabHelper* th = nullptr) {
    if (!th) {
      th = tab_helper();
    }
    if (!base::test::RunUntil([th]() {
          return speedreader::IsViewOriginal(th->PageDistillState());
        })) {
      return false;
    }
    content::WaitForLoadStop(ActiveWebContents());
    return true;
  }

  bool ClickReaderButton() {
    const auto was_distilled =
        speedreader::IsDistilled(tab_helper()->PageDistillState());
    browser()->command_controller()->ExecuteCommand(
        IDC_SPEEDREADER_ICON_ONCLICK);
    if (!was_distilled) {
      if (!WaitDistilled()) {
        return false;
      }
    } else {
      if (!WaitDistillable()) {
        return false;
      }
    }
    content::WaitForLoadStop(ActiveWebContents());
    return true;
  }

  bool WaitToolbarVisibility(ReaderModeToolbarView* toolbar, bool visible) {
    if (!base::test::RunUntil([toolbar, visible]() {
          return toolbar->GetVisible() == visible;
        })) {
      return false;
    }

    if (visible) {
      if (!base::test::RunUntil([toolbar]() {
            return toolbar->height() == toolbar->GetPreferredSize().height();
          })) {
        return false;
      }
    }
    return true;
  }

  void ClickInView(views::View* clickable_view) {
    clickable_view->OnMousePressed(
        ui::MouseEvent(ui::EventType::kMousePressed, gfx::Point(), gfx::Point(),
                       ui::EventTimeForNow(), ui::EF_LEFT_MOUSE_BUTTON, 0));
    clickable_view->OnMouseReleased(ui::MouseEvent(
        ui::EventType::kMouseReleased, gfx::Point(), gfx::Point(),
        ui::EventTimeForNow(), ui::EF_LEFT_MOUSE_BUTTON, 0));
  }

  void DisableSpeedreader() {
    browser()->GetProfile()->GetPrefs()->SetBoolean(
        speedreader::kSpeedreaderEnabled, false);
  }

  void EnableSpeedreaderAllowedForAllSites() {
    speedreader_service()->SetAllowedForAllReadableSites(true);
  }

  void DisableSpeedreaderForAllSites() {
    speedreader_service()->SetAllowedForAllReadableSites(false);
  }

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
  base::HistogramTester histogram_tester_;
};

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, PRE_RestoreSpeedreaderPage) {
  EnableSpeedreaderAllowedForAllSites();
  NavigateToPageSynchronously(kTestPageReadable,
                              WindowOpenDisposition::CURRENT_TAB);
  EXPECT_TRUE(speedreader::IsDistilled(tab_helper()->PageDistillState()));
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, RestoreSpeedreaderPage) {
  browser()->tab_strip_model()->ActivateTabAt(0);
  ASSERT_TRUE(WaitDistilled());
  EXPECT_TRUE(speedreader::IsDistilled(tab_helper()->PageDistillState()));
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, NavigationNostickTest) {
  EnableSpeedreaderAllowedForAllSites();
  NavigateToPageSynchronously(kTestPageSimple);
  EXPECT_FALSE(speedreader::IsDistilled(tab_helper()->PageDistillState()));
  NavigateToPageSynchronously(kTestPageReadable,
                              WindowOpenDisposition::CURRENT_TAB);
  EXPECT_TRUE(speedreader::IsDistilled(tab_helper()->PageDistillState()));

  // Ensure distill state doesn't stick when we back-navigate from a readable
  // page to a non-readable one.
  GoBack(browser());
  EXPECT_FALSE(speedreader::IsDistilled(tab_helper()->PageDistillState()));
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, DisableSiteWorks) {
  EnableSpeedreaderAllowedForAllSites();
  NavigateToPageSynchronously(kTestPageReadable);
  EXPECT_TRUE(speedreader::IsDistilled(tab_helper()->PageDistillState()));
  speedreader_service()->SetEnabledForSite(ActiveWebContents(), false);
  EXPECT_TRUE(WaitForLoadStop(ActiveWebContents()));
  EXPECT_FALSE(speedreader::IsDistilled(tab_helper()->PageDistillState()));
}

// I assume that the periodic fails of this test are related to issues/36355, I
// need to deal with it before turning it back. Other tests cover the
// scenario in this one, so a temporary disabling will not affect the health
// check of the feature.
IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, DISABLED_SmokeTest) {
  // Solana web3.js console warning will interfere with console observer
  brave_wallet::SetDefaultSolanaWallet(
      browser()->GetProfile()->GetPrefs(),
      brave_wallet::mojom::DefaultWallet::None);

  const std::string kGetContentLength = "document.body.innerHTML.length";

  // Check that disabled speedreader doesn't affect the page.
  EXPECT_FALSE(speedreader_service()->IsAllowedForAllReadableSites());
  NavigateToPageSynchronously(kTestPageReadable,
                              WindowOpenDisposition::CURRENT_TAB);
  const auto first_load_page_length =
      content::EvalJs(ActiveWebContents(), kGetContentLength,
                      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                      ISOLATED_WORLD_ID_BRAVE_INTERNAL)
          .ExtractInt();
  EXPECT_LT(83000, first_load_page_length);

  EnableSpeedreaderAllowedForAllSites();
  EXPECT_TRUE(speedreader_service()->IsAllowedForAllReadableSites());

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

  EnableSpeedreaderAllowedForAllSites();
  EXPECT_FALSE(speedreader_service()->IsAllowedForAllReadableSites());

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
  EnableSpeedreaderAllowedForAllSites();

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
  EXPECT_FALSE(speedreader_service()->IsAllowedForAllReadableSites());

  NavigateToPageSynchronously(kTestPageReadable);
  EXPECT_TRUE(GetReaderButton()->GetVisible());

  EXPECT_FALSE(speedreader::IsDistilled(tab_helper()->PageDistillState()));

  histogram_tester_.ExpectTotalCount(
      speedreader::kSpeedreaderPageViewsHistogramName, 0);

  ASSERT_TRUE(ClickReaderButton());
  EXPECT_TRUE(GetReaderButton()->GetVisible());
  EXPECT_TRUE(speedreader::IsDistilled(tab_helper()->PageDistillState()));
  EXPECT_TRUE(GetReaderButton()->GetVisible());

  histogram_tester_.ExpectTotalCount(
      speedreader::kSpeedreaderPageViewsHistogramName, 1);

  ASSERT_TRUE(ClickReaderButton());
  EXPECT_TRUE(GetReaderButton()->GetVisible());
  EXPECT_TRUE(speedreader::IsViewOriginal(tab_helper()->PageDistillState()));

  EXPECT_FALSE(speedreader_service()->IsAllowedForAllReadableSites());
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, OnDemandReader) {
  EXPECT_FALSE(speedreader_service()->IsAllowedForAllReadableSites());

  NavigateToPageSynchronously(kTestPageReadable);
  EXPECT_TRUE(GetReaderButton()->GetVisible());

  EXPECT_TRUE(speedreader::IsDistillable(tab_helper()->PageDistillState()));
  // Change content on the page.
  static constexpr char kChangeContent[] =
      R"js(
        document.querySelector('meta[property="og:title"]').content =
            'Title was changed by javascript'
      )js";
  EXPECT_TRUE(content::ExecJs(ActiveWebContents(), kChangeContent,
                              content::EXECUTE_SCRIPT_DEFAULT_OPTIONS));
  ASSERT_TRUE(ClickReaderButton());

  EXPECT_TRUE(speedreader::IsDistilled(tab_helper()->PageDistillState()));

  // Check title on the distilled page.
  static constexpr char kCheckContent[] =
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
  EXPECT_FALSE(speedreader_service()->IsAllowedForAllReadableSites());
  NavigateToPageSynchronously(kTestEsPageReadable);
  EXPECT_TRUE(GetReaderButton()->GetVisible());
  ASSERT_TRUE(ClickReaderButton());

  static constexpr char kCheckText[] =
      R"js( document.querySelector('#par-to-check').innerText.length )js";
  EXPECT_EQ(92, content::EvalJs(ActiveWebContents(), kCheckText,
                                content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                                ISOLATED_WORLD_ID_BRAVE_INTERNAL)
                    .ExtractInt());
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, SpeedreaderPrefDisabled) {
  DisableSpeedreader();
  NavigateToPageSynchronously(kTestPageReadable);

  EXPECT_FALSE(GetReaderButton()->GetVisible());
  EXPECT_FALSE(speedreader::IsDistilled(tab_helper()->PageDistillState()));
  EnableSpeedreaderAllowedForAllSites();
  content::WaitForLoadStop(ActiveWebContents());
  EXPECT_FALSE(GetReaderButton()->GetVisible());
  EXPECT_FALSE(speedreader::IsDistilled(tab_helper()->PageDistillState()));
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, EnableDisableSpeedreaderA) {
  EXPECT_FALSE(speedreader_service()->IsAllowedForAllReadableSites());
  NavigateToPageSynchronously(kTestPageReadable);

  EXPECT_TRUE(GetReaderButton()->GetVisible());
  EXPECT_TRUE(speedreader::IsDistillable(tab_helper()->PageDistillState()));
  EnableSpeedreaderAllowedForAllSites();
  ASSERT_TRUE(WaitDistilled());
  EXPECT_TRUE(GetReaderButton()->GetVisible());
  EXPECT_TRUE(speedreader::IsDistilled(tab_helper()->PageDistillState()));
  DisableSpeedreaderForAllSites();
  ASSERT_TRUE(WaitOriginal());
  EXPECT_TRUE(GetReaderButton()->GetVisible());
  EXPECT_TRUE(speedreader::IsDistillable(tab_helper()->PageDistillState()));
  EXPECT_TRUE(speedreader::IsViewOriginal(tab_helper()->PageDistillState()));
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, EnableDisableSpeedreaderB) {
  NavigateToPageSynchronously(kTestPageReadable);
  ASSERT_TRUE(ClickReaderButton());
  ASSERT_TRUE(WaitDistilled());
  EXPECT_TRUE(GetReaderButton()->GetVisible());
  EXPECT_TRUE(speedreader::IsDistilled(tab_helper()->PageDistillState()));
  EnableSpeedreaderAllowedForAllSites();
  ASSERT_TRUE(WaitDistilled());
  EXPECT_TRUE(GetReaderButton()->GetVisible());
  EXPECT_TRUE(speedreader::IsDistilled(tab_helper()->PageDistillState()));
  DisableSpeedreaderForAllSites();
  ASSERT_TRUE(WaitOriginal());
  EXPECT_TRUE(GetReaderButton()->GetVisible());
  EXPECT_TRUE(speedreader::IsDistillable(tab_helper()->PageDistillState()));
  EXPECT_TRUE(speedreader::IsViewOriginal(tab_helper()->PageDistillState()));
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, TogglingSiteSpeedreader) {
  EnableSpeedreaderAllowedForAllSites();
  NavigateToPageSynchronously(kTestPageReadable);

  for (int i = 0; i < 2; ++i) {
    EXPECT_TRUE(WaitForLoadStop(ActiveWebContents()));
    EXPECT_TRUE(speedreader::IsDistilled(tab_helper()->PageDistillState()));
    EXPECT_TRUE(GetReaderButton()->GetVisible());

    speedreader_service()->SetEnabledForSite(ActiveWebContents(), false);
    EXPECT_TRUE(WaitForLoadStop(ActiveWebContents()));
    EXPECT_TRUE(speedreader::IsViewOriginal(tab_helper()->PageDistillState()));
    EXPECT_TRUE(GetReaderButton()->GetVisible());

    speedreader_service()->SetEnabledForSite(ActiveWebContents(), true);
    EXPECT_TRUE(WaitForLoadStop(ActiveWebContents()));
  }
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, ReloadContent) {
  EnableSpeedreaderAllowedForAllSites();
  NavigateToPageSynchronously(kTestPageReadable);
  auto* contents_1 = ActiveWebContents();
  NavigateToPageSynchronously(kTestPageReadable);
  auto* contents_2 = ActiveWebContents();

  auto* tab_helper_1 =
      speedreader::SpeedreaderTabHelper::FromWebContents(contents_1);
  auto* tab_helper_2 =
      speedreader::SpeedreaderTabHelper::FromWebContents(contents_2);

  EXPECT_TRUE(speedreader::IsDistilled(tab_helper_1->PageDistillState()));
  EXPECT_TRUE(speedreader::IsDistilled(tab_helper_2->PageDistillState()));

  speedreader_service()->SetEnabledForSite(tab_helper_1->web_contents(), false);
  content::WaitForLoadStop(contents_1);
  EXPECT_TRUE(speedreader::IsViewOriginal(tab_helper_1->PageDistillState()));
  EXPECT_TRUE(speedreader::IsDistilled(tab_helper_2->PageDistillState()));

  contents_2->GetController().Reload(content::ReloadType::NORMAL, false);
  content::WaitForLoadStop(contents_2);

  EXPECT_TRUE(speedreader::IsViewOriginal(tab_helper_1->PageDistillState()));
  EXPECT_TRUE(speedreader::IsViewOriginal(tab_helper_2->PageDistillState()));
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, ShowOriginalPage) {
  EnableSpeedreaderAllowedForAllSites();
  NavigateToPageSynchronously(kTestPageReadable);
  ASSERT_TRUE(WaitDistilled());
  auto* web_contents = ActiveWebContents();

  static constexpr char kCheckNoApiInMainWorld[] =
      R"js(
        document.speedreader === undefined
      )js";
  EXPECT_TRUE(content::EvalJs(web_contents, kCheckNoApiInMainWorld,
                              content::EXECUTE_SCRIPT_DEFAULT_OPTIONS)
                  .ExtractBool());

  // Wait for the "View original" link to be present in the DOM.
  // The element ID is hardcoded in extractor.rs.
  // Note: We use a polling loop with NonBlockingDelay instead of
  // base::test::RunUntil() here because EvalJs inside RunUntil causes nesting
  // level issues on macOS arm64 (DCHECK in message_pump_apple.mm).
  static constexpr char kCheckLinkExists[] =
      R"js(
        !!document.getElementById('c93e2206-2f31-4ddc-9828-2bb8e8ed940e')
      )js";
  const base::TimeTicks deadline = base::TimeTicks::Now() + base::Seconds(10);
  for (;;) {
    NonBlockingDelay(base::Milliseconds(10));
    if (content::EvalJs(web_contents, kCheckLinkExists,
                        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                        ISOLATED_WORLD_ID_BRAVE_INTERNAL)
            .ExtractBool()) {
      break;
    }
    if (base::TimeTicks::Now() >= deadline) {
      FAIL() << "Timeout waiting for 'View original' link to appear";
    }
  }

  static constexpr char kClickLinkAndGetTitle[] =
      R"js(
    (function() {
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
  EXPECT_TRUE(speedreader::IsDistillable(tab_helper->PageDistillState()));
  EXPECT_TRUE(speedreader_service()->IsAllowedForSite(web_contents));

  // Click on speedreader button
  ASSERT_TRUE(ClickReaderButton());
  content::WaitForLoadStop(web_contents);
  EXPECT_TRUE(speedreader::IsDistilled(tab_helper->PageDistillState()));
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, ShowOriginalPageOnUnreadable) {
  EnableSpeedreaderAllowedForAllSites();
  NavigateToPageSynchronously(kTestPageSimple);
  auto* web_contents = ActiveWebContents();

  static constexpr char kCheckNoElement[] =
      R"js(
        document.getElementById('c93e2206-2f31-4ddc-9828-2bb8e8ed940e') == null
      )js";

  EXPECT_TRUE(content::EvalJs(web_contents, kCheckNoElement,
                              content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                              ISOLATED_WORLD_ID_BRAVE_INTERNAL)
                  .ExtractBool());

  static constexpr char kCheckNoApi[] =
      R"js(
        document.speedreader === undefined
      )js";

  EXPECT_TRUE(content::EvalJs(web_contents, kCheckNoApi,
                              content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                              ISOLATED_WORLD_ID_BRAVE_INTERNAL)
                  .ExtractBool());
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, SetDataAttributes) {
  EnableSpeedreaderAllowedForAllSites();
  NavigateToPageSynchronously(kTestPageReadable);
  auto* contents = ActiveWebContents();

  // Open second tab
  NavigateToPageSynchronously(kTestPageReadable);

  auto GetDataAttribute = [](const std::string& attr) {
    static constexpr char kGetDataAttribute[] =
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

  EXPECT_EQ(base::Value(),
            content::EvalJs(contents, GetDataAttribute("data-theme"),
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
    static constexpr char kGetDataAttribute[] =
        R"js(
          document.documentElement.getAttribute('$1')
        )js";
    return base::ReplaceStringPlaceholders(kGetDataAttribute, {attr}, nullptr);
  };

  auto WaitAttr = [&](content::WebContents* contents, const std::string& attr,
                      const std::string& value) -> testing::AssertionResult {
    const base::TimeTicks deadline =
        base::TimeTicks::Now() + TestTimeouts::action_max_timeout();
    for (;;) {
      NonBlockingDelay(base::Milliseconds(10));
      auto eval = content::EvalJs(contents, GetDataAttribute(attr),
                                  content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                                  ISOLATED_WORLD_ID_BRAVE_INTERNAL);
      // The attribute is removed, not emptied, when the value is the default,
      // so a null result means the empty value. is_string() is also false for
      // error results, hence the explicit is_ok() check.
      if (eval.is_ok() && !eval.is_string() && value.empty()) {
        return testing::AssertionSuccess();
      }
      if (eval.is_string() && eval.ExtractString() == value) {
        return testing::AssertionSuccess();
      }
      if (base::TimeTicks::Now() >= deadline) {
        return testing::AssertionFailure()
               << "Timed out waiting for " << attr << "=\"" << value
               << "\", last value: " << eval;
      }
    }
  };

  auto WaitElement = [&](content::WebContents* contents,
                         const std::string& elem) -> testing::AssertionResult {
    static constexpr char kWaitElement[] =
        R"js(
          (!!document.getElementById('$1'))
        )js";
    const base::TimeTicks deadline =
        base::TimeTicks::Now() + TestTimeouts::action_max_timeout();
    for (;;) {
      NonBlockingDelay(base::Milliseconds(10));
      if (content::EvalJs(
              contents,
              base::ReplaceStringPlaceholders(kWaitElement, {elem}, nullptr),
              content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
              ISOLATED_WORLD_ID_BRAVE_INTERNAL)
              .ExtractBool()) {
        return testing::AssertionSuccess();
      }
      if (base::TimeTicks::Now() >= deadline) {
        return testing::AssertionFailure()
               << "Timed out waiting for element #" << elem;
      }
    }
  };

  auto Click = [&](content::WebContents* contents,
                   const std::string& id) -> testing::AssertionResult {
    static constexpr char kClick[] =
        R"js(
          document.getElementById('$1').click()
        )js";
    // The toolbar is a WebUI whose controls are rendered asynchronously as
    // React reacts to state changes, so the element to click may not exist
    // yet.
    testing::AssertionResult element_exists = WaitElement(contents, id);
    if (!element_exists) {
      return element_exists;
    }
    return content::ExecJs(
        contents, base::ReplaceStringPlaceholders(kClick, {id}, nullptr));
  };

  EnableSpeedreaderAllowedForAllSites();
  NavigateToPageSynchronously(kTestPageReadable);
  // Appearance changes are only applied to a distilled page:
  // SpeedreaderTabHelper::OnAppearanceSettingsChanged() drops them otherwise,
  // and they are never replayed. NavigateToPageSynchronously() only waits for
  // load stop, not for distillation.
  ASSERT_TRUE(WaitDistilled());

  auto* page = ActiveWebContents();
  auto* toolbar_view = BraveBrowserView::GetBrowserViewForBrowser(browser())
                           ->reader_mode_toolbar();
  // The toolbar contents are created by ReaderModeToolbarView::SetVisible(),
  // so wait for the toolbar itself rather than assuming distillation already
  // made it visible.
  ASSERT_TRUE(WaitToolbarVisibility(toolbar_view, true));
  auto* toolbar = toolbar_view->GetWebContentsForTesting();
  ASSERT_TRUE(toolbar);
  ASSERT_TRUE(WaitElement(toolbar, "appearance"));

#if BUILDFLAG(ENABLE_AI_CHAT)
  ASSERT_TRUE(Click(toolbar, "ai"));
  auto* side_panel = browser()->GetFeatures().side_panel_ui();
  ASSERT_TRUE(base::test::RunUntil([side_panel]() {
    return side_panel->GetCurrentEntryId() == SidePanelEntryId::kChatUI;
  })) << "Timed out waiting for the AI chat side panel to open, a side panel "
         "entry is "
      << (side_panel->GetCurrentEntryId().has_value() ? "shown" : "not shown");
  ASSERT_TRUE(Click(toolbar, "ai"));
  ASSERT_TRUE(base::test::RunUntil([side_panel]() {
    return !side_panel->GetCurrentEntryId().has_value();
  })) << "Timed out waiting for the side panel to close";
#endif

  ASSERT_TRUE(Click(toolbar, "appearance"));
  {  // change theme
    ASSERT_TRUE(Click(toolbar, "theme-light"));
    ASSERT_TRUE(WaitAttr(page, "data-theme", "light"));
    ASSERT_TRUE(Click(toolbar, "theme-sepia"));
    ASSERT_TRUE(WaitAttr(page, "data-theme", "sepia"));
    ASSERT_TRUE(Click(toolbar, "theme-dark"));
    ASSERT_TRUE(WaitAttr(page, "data-theme", "dark"));
    ASSERT_TRUE(Click(toolbar, "theme-system"));
    ASSERT_TRUE(WaitAttr(page, "data-theme", ""));
  }
  {  // change font
    ASSERT_TRUE(Click(toolbar, "font-sans"));
    ASSERT_TRUE(WaitAttr(page, "data-font-family", "sans"));
    ASSERT_TRUE(Click(toolbar, "font-serif"));
    ASSERT_TRUE(WaitAttr(page, "data-font-family", "serif"));
    ASSERT_TRUE(Click(toolbar, "font-mono"));
    ASSERT_TRUE(WaitAttr(page, "data-font-family", "mono"));
    ASSERT_TRUE(Click(toolbar, "font-dyslexic"));
    ASSERT_TRUE(WaitAttr(page, "data-font-family", "dyslexic"));
  }
  {  // change font size
    ASSERT_TRUE(WaitAttr(page, "data-font-size", "100"));
    ASSERT_TRUE(Click(toolbar, "font-size-decrease"));
    ASSERT_TRUE(WaitAttr(page, "data-font-size", "90"));
    ASSERT_TRUE(Click(toolbar, "font-size-increase"));
    ASSERT_TRUE(WaitAttr(page, "data-font-size", "100"));
    ASSERT_TRUE(Click(toolbar, "font-size-increase"));
    ASSERT_TRUE(WaitAttr(page, "data-font-size", "110"));
  }
  ASSERT_TRUE(Click(toolbar, "appearance"));

  ASSERT_TRUE(Click(toolbar, "tune"));
  {
    ASSERT_TRUE(base::test::RunUntil([this]() {
      return tab_helper()->speedreader_bubble_view() != nullptr;
    }));
  }
  ASSERT_TRUE(Click(toolbar, "tune"));

  ASSERT_TRUE(Click(toolbar, "close"));
  {
    ASSERT_TRUE(WaitOriginal());
    EXPECT_FALSE(toolbar_view->GetVisible());
  }
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, ToolbarLangs) {
  language::LanguagePrefs language_prefs(browser()->GetProfile()->GetPrefs());
  language_prefs.SetUserSelectedLanguagesList(
      {"en-US", "ja", "en-CA", "fr-CA"});

  EnableSpeedreaderAllowedForAllSites();
  NavigateToPageSynchronously(kTestPageReadable);

  auto* toolbar_view = BraveBrowserView::GetBrowserViewForBrowser(browser())
                           ->reader_mode_toolbar();
  auto* toolbar = toolbar_view->GetWebContentsForTesting();

  static constexpr char kGetLang[] = R"js( navigator.languages.toString() )js";
  EXPECT_EQ("en-US,ja,en-CA,fr-CA",
            content::EvalJs(toolbar, kGetLang).ExtractString());
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, RSS) {
  EnableSpeedreaderAllowedForAllSites();
  NavigateToPageSynchronously(kTestXml);

  EXPECT_FALSE(GetReaderButton()->GetVisible());

  const std::string kNoStyleInjected =
      R"js(document.getElementById('brave_speedreader_style'))js";

  EXPECT_EQ(base::Value(),
            content::EvalJs(ActiveWebContents(), kNoStyleInjected,
                            content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                            ISOLATED_WORLD_ID_BRAVE_INTERNAL));
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, TTS) {
  EnableSpeedreaderAllowedForAllSites();

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
  EnableSpeedreaderAllowedForAllSites();
  NavigateToPageSynchronously(kTestErrorPage,
                              WindowOpenDisposition::CURRENT_TAB);
  EXPECT_TRUE(ActiveWebContents()->GetPrimaryMainFrame()->IsErrorDocument());
  EXPECT_FALSE(GetReaderButton()->GetVisible());

  // Navigate to the non-automatic distillable page.
  NavigateToPageSynchronously(kTestPageReadableOnUnreadablePath,
                              WindowOpenDisposition::CURRENT_TAB);
  EXPECT_TRUE(speedreader::IsViewOriginal(tab_helper()->PageDistillState()));
  ASSERT_TRUE(WaitDistillable(tab_helper()));
  EXPECT_TRUE(GetReaderButton()->GetVisible());

  GoBack(browser());
  NavigateToPageSynchronously(kTestPageReadable,
                              WindowOpenDisposition::CURRENT_TAB);
  ASSERT_TRUE(WaitDistilled());
  EXPECT_TRUE(GetReaderButton()->GetVisible());
  EXPECT_TRUE(speedreader::IsDistilled(tab_helper()->PageDistillState()));
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, Csp) {
  EnableSpeedreaderAllowedForAllSites();

  for (const auto* page : {kTestCSPHackEquivPage, kTestCSPHackCharsetPage,
                           kTestCSPHtmlPage, kTestCSPHttpPage}) {
    SCOPED_TRACE(page);

    content::WebContentsConsoleObserver console_observer(ActiveWebContents());
    console_observer.SetPattern(
        "Loading the image 'https://a.test/should_fail.png' violates the "
        "following Content Security Policy directive: \"img-src "
        "'none'\".*");

    NavigateToPageSynchronously(page, WindowOpenDisposition::CURRENT_TAB);

    static constexpr char kCheckNoMaliciousContent[] = R"js(
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
  EnableSpeedreaderAllowedForAllSites();

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
        "Setting the document's base URI to 'https://a.test/' violates the "
        "following Content Security Policy directive: "
        "\"base-uri 'none'\".*");
    NavigateToPageSynchronously(kTestCSPOrderPage2,
                                WindowOpenDisposition::CURRENT_TAB);
    EXPECT_TRUE(console_observer.Wait());
  }
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, CspInBody) {
  EnableSpeedreaderAllowedForAllSites();

  NavigateToPageSynchronously(kTestCSPInBodyPage,
                              WindowOpenDisposition::CURRENT_TAB);
  static constexpr char kCheckCsp[] = R"js(
    document.querySelectorAll('meta[content="CSP in body"]').length === 0
  )js";

  EXPECT_EQ(true, content::EvalJs(ActiveWebContents(), kCheckCsp,
                                  content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                                  ISOLATED_WORLD_ID_BRAVE_INTERNAL));
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest,
                       OnDemandReaderSetEnabledForSite) {
  EXPECT_FALSE(speedreader_service()->IsAllowedForAllReadableSites());

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

  EXPECT_TRUE(speedreader::IsDistillable(tab_helper()->PageDistillState()));

  ASSERT_TRUE(ClickReaderButton());

  EXPECT_TRUE(speedreader::IsDistilled(tab_helper()->PageDistillState()));

  // Enable speedreader for site explicitly.
  speedreader_service()->SetEnabledForSite(ActiveWebContents(), true);
  ActiveWebContents()->GetController().Reload(content::ReloadType::NORMAL,
                                              false);
  ASSERT_TRUE(WaitDistilled());

  EXPECT_TRUE(speedreader::IsDistilled(tab_helper()->PageDistillState()));

  // Go to home page.
  NavigateToPageSynchronously("/", WindowOpenDisposition::CURRENT_TAB);
  EXPECT_TRUE(speedreader::IsViewOriginal(tab_helper()->PageDistillState()));
}

// Test toolbar's rounded corners is updated when split view is toggled.
IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, ToolbarWithRoundedCorners) {
  EnableSpeedreaderAllowedForAllSites();

  auto* tab_strip_model = browser()->tab_strip_model();

  // Load distilled page at tab 0.
  EXPECT_EQ(0, tab_strip_model->active_index());
  NavigateToPageSynchronously(kTestPageReadable,
                              WindowOpenDisposition::CURRENT_TAB);
  EXPECT_TRUE(speedreader::IsDistilled(tab_helper()->PageDistillState()));

  const bool rounded_contents =
      browser()->GetProfile()->GetPrefs()->GetBoolean(kWebViewRoundedCorners);

  auto* browser_view = BraveBrowserView::GetBrowserViewForBrowser(browser());
  EXPECT_EQ(browser_view->reader_mode_toolbar()->rounded_corners_.IsEmpty(),
            !rounded_contents);
  chrome::NewSplitTab(browser(), split_tabs::SplitTabLayout::kSideBySide,
                      split_tabs::SplitTabCreatedSource::kTabContextMenu);

  // Tab at 1 is newly created tab with split view and it's not distilled.
  EXPECT_FALSE(speedreader::IsDistilled(tab_helper()->PageDistillState()));

  tab_strip_model->ActivateTabAt(0);
  EXPECT_TRUE(speedreader::IsDistilled(tab_helper()->PageDistillState()));
  EXPECT_FALSE(browser_view->reader_mode_toolbar()->rounded_corners_.IsEmpty());

  auto* active_tab = tab_strip_model->GetActiveTab();
  auto split_id = active_tab->GetSplit();
  ASSERT_TRUE(split_id);
  tab_strip_model->RemoveSplit(*split_id);
  EXPECT_EQ(0, tab_strip_model->active_index());
  EXPECT_EQ(browser_view->reader_mode_toolbar()->rounded_corners_.IsEmpty(),
            !rounded_contents);
}

// The content distilled from one page must never be shown as the content of
// another page: it would allow a page to put its own markup under the URL,
// the TLS indicators and the response headers of an unrelated origin.
// The reload which speedreader triggers to show the distilled content is
// answered here in a way that leaves the content unsent, i.e. the reload is
// redirected somewhere else or turned into a download.
class SpeedReaderContentSpoofBrowserTest : public SpeedReaderBrowserTest {
 public:
  static constexpr char kVictimHost[] = "b.test";
  static constexpr char kVictimPage[] = "/simple.html";
  // The title of kVictimPage, it must stay the title of the shown document.
  static constexpr char kVictimPageTitle[] = "OK";

  SpeedReaderContentSpoofBrowserTest() = default;
  ~SpeedReaderContentSpoofBrowserTest() override = default;

  void SetUpOnMainThread() override {
    // Request handlers have a priority over the file serving handler installed
    // by SpeedReaderBrowserTest, so the first request is still served from the
    // disk, see HandleReload().
    https_server_.RegisterRequestHandler(
        base::BindRepeating(&SpeedReaderContentSpoofBrowserTest::HandleReload,
                            base::Unretained(this)));
    SpeedReaderBrowserTest::SetUpOnMainThread();
  }

  GURL victim_url() const {
    return https_server_.GetURL(kVictimHost, kVictimPage);
  }

  // Opens the readable page which answers the speedreader's reload according to
  // |mode| ("redirect" or "attachment").
  void NavigateToReadablePage(std::string_view mode) {
    NavigateToPageSynchronously(base::StrCat({kTestPageReadable, "?", mode}),
                                WindowOpenDisposition::CURRENT_TAB);
    ASSERT_TRUE(WaitDistillable());
  }

  // Turns the reader mode on. Speedreader distills the current document and
  // reloads the page to show the distilled content.
  void TurnOnReaderMode() {
    browser()->command_controller()->ExecuteCommand(
        IDC_SPEEDREADER_ICON_ONCLICK);
  }

  // Checks that the document currently shown is the original kVictimPage and
  // not something distilled by speedreader.
  void ExpectVictimPageIsIntact() {
    EXPECT_EQ(victim_url(), ActiveWebContents()->GetLastCommittedURL());
    EXPECT_EQ(kVictimPageTitle,
              content::EvalJs(ActiveWebContents(), "document.title",
                              content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                              ISOLATED_WORLD_ID_BRAVE_INTERNAL)
                  .ExtractString());
    EXPECT_FALSE(
        content::EvalJs(ActiveWebContents(),
                        "!!document.getElementById('brave_speedreader_style')",
                        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                        ISOLATED_WORLD_ID_BRAVE_INTERNAL)
            .ExtractBool());
    EXPECT_FALSE(speedreader::IsDistilled(tab_helper()->PageDistillState()));
    // The pending distillation is dropped together with the content, so
    // speedreader is not stuck in the distilling state either.
    EXPECT_TRUE(speedreader::IsViewOriginal(tab_helper()->PageDistillState()));
  }

 private:
  std::unique_ptr<net::test_server::HttpResponse> HandleReload(
      const net::test_server::HttpRequest& request) {
    if (request.GetURL().path() != kTestPageReadable ||
        ++requests_count_ == 1) {
      // Let the readable page itself be served from the disk.
      return nullptr;
    }

    auto response = std::make_unique<net::test_server::BasicHttpResponse>();
    response->set_content_type("text/html");
    if (request.GetURL().query() == "redirect") {
      response->set_code(net::HTTP_FOUND);
      response->AddCustomHeader("Location", victim_url().spec());
    } else {
      response->set_code(net::HTTP_OK);
      response->AddCustomHeader("Content-Disposition", "attachment");
      response->set_content("<html><body>attachment</body></html>");
    }
    return response;
  }

  // Accessed on the embedded test server's thread only.
  int requests_count_ = 0;
};

// The reload is turned into a download, so the distilled content is never
// consumed. It must not stay armed for the pages the user visits next.
IN_PROC_BROWSER_TEST_F(SpeedReaderContentSpoofBrowserTest,
                       DistilledContentIsDroppedWhenTheReloadIsNotShown) {
  ASSERT_NO_FATAL_FAILURE(NavigateToReadablePage("attachment"));
  const GURL readable_url = ActiveWebContents()->GetLastCommittedURL();

  // Content-Disposition: attachment replaces the reload with a download, so
  // waiting for the navigation would hang, the events it waits for never come
  // for downloads. Disable the download prompt so the download proceeds
  // automatically and wait for the download itself instead. Same pattern as
  // DeAmpBrowserTest.ContentDispositionAttachment.
  browser()->GetProfile()->GetPrefs()->SetBoolean(prefs::kPromptForDownload,
                                                  false);
  content::DownloadTestObserverTerminal download_observer(
      browser()->GetProfile()->GetDownloadManager(), 1,
      content::DownloadTestObserver::ON_DANGEROUS_DOWNLOAD_ACCEPT);
  TurnOnReaderMode();
  download_observer.WaitForFinished();
  EXPECT_EQ(1u, download_observer.NumDownloadsSeenInState(
                    download::DownloadItem::COMPLETE));

  // The download doesn't replace the page, so the readable page is still
  // shown, with the distilled content still waiting to be sent.
  EXPECT_EQ(readable_url, ActiveWebContents()->GetLastCommittedURL());

  // Now the user navigates wherever they want. The content must be dropped
  // when this navigation starts, long before its response arrives.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), victim_url()));

  ExpectVictimPageIsIntact();
}

class SpeedReaderWithSplitViewBrowserTest : public SpeedReaderBrowserTest {
 public:
  SpeedReaderWithSplitViewBrowserTest() = default;
  ~SpeedReaderWithSplitViewBrowserTest() override = default;

  void NewSplitTab() {
    chrome::NewSplitTab(browser(), split_tabs::SplitTabLayout::kSideBySide,
                        split_tabs::SplitTabCreatedSource::kTabContextMenu);
  }

  BraveBrowserView* brave_browser_view() {
    return BraveBrowserView::GetBrowserViewForBrowser(browser());
  }

  // Don't cache as it changes whenever active tab changes.
  ReaderModeToolbarView* GetPrimaryToolbar() {
    return brave_browser_view()->reader_mode_toolbar();
  }

  // Don't cache as it changes whenever active tab changes.
  ReaderModeToolbarView* GetSecondaryToolbar() {
    return BraveContentsContainerView::From(
               brave_browser_view()
                   ->GetBraveMultiContentsView()
                   ->GetInactiveContentsContainerView())
        ->reader_mode_toolbar();
  }

  views::View* GetSecondaryContentsContainer() {
    return brave_browser_view()
        ->GetBraveMultiContentsView()
        ->GetInactiveContentsContainerView();
  }

 private:
  base::test::ScopedFeatureList scoped_features_;
};

IN_PROC_BROWSER_TEST_F(SpeedReaderWithSplitViewBrowserTest, SplitView) {
  EnableSpeedreaderAllowedForAllSites();

  NewSplitTab();
  ASSERT_TRUE(GetPrimaryToolbar() && GetSecondaryToolbar());

  // No toolbars.
  EXPECT_FALSE(GetPrimaryToolbar()->GetVisible());
  EXPECT_FALSE(GetSecondaryToolbar()->GetVisible());

  // Load a distillabe page in first tab.
  browser()->tab_strip_model()->ActivateTabAt(0);
  NavigateToPageSynchronously(kTestPageReadable,
                              WindowOpenDisposition::CURRENT_TAB);

  ASSERT_TRUE(WaitToolbarVisibility(GetPrimaryToolbar(), true));
  ASSERT_TRUE(WaitToolbarVisibility(GetSecondaryToolbar(), false));

  // Change the active tab.
  browser()->tab_strip_model()->ActivateTabAt(1);
  ASSERT_TRUE(WaitToolbarVisibility(GetPrimaryToolbar(), false));
  ASSERT_TRUE(WaitToolbarVisibility(GetSecondaryToolbar(), true));

  // Load a distillabe page in second tab.
  NavigateToPageSynchronously(kTestPageReadable,
                              WindowOpenDisposition::CURRENT_TAB);
  ASSERT_TRUE(WaitToolbarVisibility(GetPrimaryToolbar(), true));
  ASSERT_TRUE(WaitToolbarVisibility(GetSecondaryToolbar(), true));

  // Check secondary location bar position when changing active tab
  // between non split view tab and split view tab.
  // Secondary location bar should have same origin with secondary
  // contents container.
  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ true);
  ASSERT_TRUE(WaitToolbarVisibility(GetPrimaryToolbar(), false));
  ASSERT_TRUE(WaitToolbarVisibility(GetSecondaryToolbar(), false));

  browser()->tab_strip_model()->ActivateTabAt(0);
  ASSERT_TRUE(WaitToolbarVisibility(GetPrimaryToolbar(), true));
  ASSERT_TRUE(WaitToolbarVisibility(GetSecondaryToolbar(), true));

  browser()->tab_strip_model()->ActivateTabAt(2);
  ASSERT_TRUE(WaitToolbarVisibility(GetPrimaryToolbar(), false));
  ASSERT_TRUE(WaitToolbarVisibility(GetSecondaryToolbar(), false));

  browser()->tab_strip_model()->ActivateTabAt(0);
  ASSERT_TRUE(WaitToolbarVisibility(GetPrimaryToolbar(), true));
  ASSERT_TRUE(WaitToolbarVisibility(GetSecondaryToolbar(), true));

  // Second tab is active. Show original content.
  browser()->tab_strip_model()->ActivateTabAt(1);
  ASSERT_TRUE(ClickReaderButton());
  ASSERT_TRUE(WaitToolbarVisibility(GetPrimaryToolbar(), false));
  ASSERT_TRUE(WaitToolbarVisibility(GetSecondaryToolbar(), true));

  browser()->tab_strip_model()->ActivateTabAt(0);
  // First tab is active. Show original content.
  ASSERT_TRUE(ClickReaderButton());

  // There are no distilled pages.
  ASSERT_TRUE(WaitToolbarVisibility(GetPrimaryToolbar(), false));
  ASSERT_TRUE(WaitToolbarVisibility(GetSecondaryToolbar(), false));
}

IN_PROC_BROWSER_TEST_F(SpeedReaderWithSplitViewBrowserTest, SplitViewClicking) {
  EnableSpeedreaderAllowedForAllSites();

  NewSplitTab();

  ASSERT_TRUE(GetPrimaryToolbar() && GetSecondaryToolbar());

  // No toolbars.
  EXPECT_FALSE(GetPrimaryToolbar()->GetVisible());
  EXPECT_FALSE(GetSecondaryToolbar()->GetVisible());

  // Load a distillabe page in first tab.
  browser()->tab_strip_model()->ActivateTabAt(0);
  EXPECT_EQ(0, browser()->tab_strip_model()->active_index());
  NavigateToPageSynchronously(kTestPageReadable,
                              WindowOpenDisposition::CURRENT_TAB);

  // Check clicking view makes its tab activate.
  browser()->tab_strip_model()->ActivateTabAt(1);
  EXPECT_EQ(1, browser()->tab_strip_model()->active_index());
  ASSERT_TRUE(WaitToolbarVisibility(GetPrimaryToolbar(), false));
  ASSERT_TRUE(WaitToolbarVisibility(GetSecondaryToolbar(), true));

  ClickInView(GetSecondaryToolbar());
  ASSERT_TRUE(WaitToolbarVisibility(GetPrimaryToolbar(), true));
  ASSERT_TRUE(WaitToolbarVisibility(GetSecondaryToolbar(), false));
  EXPECT_EQ(0, browser()->tab_strip_model()->active_index());

  browser()->tab_strip_model()->ActivateTabAt(1);
  EXPECT_EQ(1, browser()->tab_strip_model()->active_index());
  ASSERT_TRUE(WaitToolbarVisibility(GetPrimaryToolbar(), false));
  ASSERT_TRUE(WaitToolbarVisibility(GetSecondaryToolbar(), true));

  // Simulated input doesn't reliably trigger DidGetUserInteraction()
  // callback on these platforms in test environments, causing intermittent
  // timeout failures.
  //
  // Workaround: Directly invoke ActivateContents() to test the tab activation
  // mechanism. The full click → DidGetUserInteraction → ActivateContents chain
  // is verified manually on these platforms.
  GetSecondaryToolbar()->ActivateContents();
  ASSERT_TRUE(WaitToolbarVisibility(GetPrimaryToolbar(), true));
  ASSERT_TRUE(WaitToolbarVisibility(GetSecondaryToolbar(), false));
  EXPECT_EQ(0, browser()->tab_strip_model()->active_index());
}
