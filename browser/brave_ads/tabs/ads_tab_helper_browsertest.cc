/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/tabs/ads_tab_helper.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "base/callback_list.h"
#include "base/check.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/notreached.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "base/test/gmock_callback_support.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/components/brave_ads/browser/ads_service_mock.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/constants/brave_paths.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/keep_alive/profile_keep_alive_types.h"
#include "chrome/browser/profiles/keep_alive/scoped_profile_keep_alive.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_restore_test_helper.h"
#include "chrome/browser/sessions/session_restore_test_utils.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/platform_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/keep_alive_registry/keep_alive_types.h"
#include "components/keep_alive_registry/scoped_keep_alive.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_service.h"
#include "components/sessions/content/session_tab_helper.h"
#include "components/sessions/core/session_id.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/media_player_id.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_status_code.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// npm run test -- brave_browser_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kHostName[] = "brave.com";

constexpr char kHandleRequestUrlPath[] = "/handle_request";
constexpr char kHttpStatusCodeQueryKey[] = "http_status_code";

constexpr char kMultiPageApplicationWebpage[] =
    "/brave_ads/multi_page_application.html";
constexpr char kMultiPageApplicationWebpageHtmlContent[] =
    "<!DOCTYPE html><html xmlns=\"http://www.w3.org/1999/xhtml\" "
    "lang=\"en\"><head>\n  <title>Adventure "
    "Awaits</title>\n</head>\n\n<body>\n  <h1>Welcome to Your Adventure</h1>\n "
    " <p>\n    Embark on a journey of learning and discovery. Each step you "
    "take brings you closer to mastering new skills and\n    achieving your "
    "goals.\n  </p>\n  <ul>\n    <li><a href=\"rust.html\" "
    "target=\"_self\">Explore new programming languages</a></li>\n    <li><a "
    "href=\"open_source.html\" target=\"_self\">Contribute to open-source "
    "projects</a></li>\n    <li><a href=\"develop.html\" "
    "target=\"_self\">Develop innovative applications</a></li>\n  </ul>\n  "
    "<blockquote>\n    \"The only limit to our realization of tomorrow is our "
    "doubts of today.\" - Franklin D. Roosevelt\n  </blockquote>\n  <table "
    "border=\"1\">\n    <tbody><tr>\n      <th>Task</th>\n      "
    "<th>Status</th>\n    </tr>\n    <tr>\n      <td>Learn Rust</td>\n      "
    "<td>Completed</td>\n    </tr>\n    <tr>\n      <td>Contribute to a GitHub "
    "repository</td>\n      <td>In Progress</td>\n    </tr>\n    <tr>\n      "
    "<td>Build a mobile app</td>\n      <td>Pending</td>\n    </tr>\n  "
    "</tbody></table>\n\n\n\n</body></html>";
constexpr char kMultiPageApplicationWebpageTextContent[] =
    "Welcome to Your Adventure\n\nEmbark on a journey of learning and "
    "discovery. Each step you take brings you closer to mastering new skills "
    "and achieving your goals.\n\nExplore new programming "
    "languages\nContribute to open-source projects\nDevelop innovative "
    "applications\n\"The only limit to our realization of tomorrow is our "
    "doubts of today.\" - Franklin D. Roosevelt\nTask\tStatus\nLearn "
    "Rust\tCompleted\nContribute to a GitHub repository\tIn Progress\nBuild a "
    "mobile app\tPending";

constexpr char kSinglePageApplicationWebpage[] =
    "/brave_ads/single_page_application.html";
constexpr char kSinglePageApplicationWebpageHtmlContent[] =
    "<!DOCTYPE html><html xmlns=\"http://www.w3.org/1999/xhtml\" "
    "lang=\"en\"><head>\n  <title>Single Page Application</title>\n  "
    "<script>\n    // Function to update the page header.\n    function "
    "displayContent(state) {\n      const pageHeader = "
    "document.querySelector(\"#pageHeader\");\n      pageHeader.textContent = "
    "state.header;\n    }\n\n    // Event listener for clicks on the "
    "document.\n    document.addEventListener(\"click\", async (event) =&gt; "
    "{\n      const navigationType = "
    "event.target.getAttribute(\"data-navigation-type\");\n      if "
    "(navigationType) {\n        event.preventDefault(); // Stop the default "
    "link behavior.\n        if (navigationType === \"same_document\") {\n     "
    "     try {\n            // Update the header.\n            "
    "displayContent({ header: navigationType });\n\n            // Change the "
    "URL without reloading.\n            const newState = { header: "
    "navigationType };\n            history.pushState(newState, \"\", "
    "navigationType);\n          } catch (err) {\n            // Log any "
    "errors.\n            console.error(err);\n          }\n        }\n      "
    "}\n    });\n\n    // Event listener for browser navigation "
    "(back/forward).\n    window.addEventListener(\"popstate\", (event) =&gt; "
    "{\n      if (event.state) {\n        // Update the header based on the "
    "state.\n        displayContent(event.state);\n      }\n    });\n\n    // "
    "Set the initial state of the page.\n    const initialState = { header: "
    "\"Home\" };\n    history.replaceState(initialState, \"\", "
    "document.location.href);\n  </script>\n</head>\n\n<body>\n  <h1 "
    "id=\"pageHeader\">same_document</h1>\n  <ul>\n    <li><a href=\"/\" "
    "data-navigation-type=\"home\">Home</a></li>\n    <li><a "
    "href=\"same_document\" data-navigation-type=\"same_document\">Same "
    "Document</a></li>\n  </ul>\n\n\n\n</body></html>";
constexpr char kSinglePageApplicationClickSelectors[] =
    "[data-navigation-type='same_document']";

constexpr char kAutoplayVideoWebpage[] = "/brave_ads/autoplay_video.html";
constexpr char kVideoWebpage[] = "/brave_ads/video.html";
constexpr char kVideoJavascriptDocumentQuerySelectors[] = "video";

MATCHER_P(FileName, filename, "") {
  return arg.ExtractFileName() == filename;
}

class MediaWaiter : public content::WebContentsObserver {
 public:
  explicit MediaWaiter(content::WebContents* const web_contents)
      : content::WebContentsObserver(web_contents) {}

  void WaitForMediaStartedPlaying() { media_started_playing_run_loop_.Run(); }

  void WaitForMediaDestroyed() { media_destroyed_run_loop_.Run(); }

  void WaitForMediaSessionCreated() { media_session_created_run_loop_.Run(); }

  // content::WebContentsObserver:
  void MediaStartedPlaying(const MediaPlayerInfo& /*video_type*/,
                           const content::MediaPlayerId& id) override {
    id_ = id;
    media_started_playing_run_loop_.Quit();
  }

  void MediaDestroyed(const content::MediaPlayerId& id) override {
    EXPECT_EQ(id, id_);
    media_destroyed_run_loop_.Quit();
  }

  void MediaSessionCreated(
      content::MediaSession* const /*media_session*/) override {
    media_session_created_run_loop_.Quit();
  }

 private:
  std::optional<content::MediaPlayerId> id_;

  base::RunLoop media_started_playing_run_loop_;
  base::RunLoop media_destroyed_run_loop_;
  base::RunLoop media_session_created_run_loop_;
};

}  // namespace

// We expect `is_visible=true` if both the browser and tab are active, and
// `is_visible=false` if either the browser or tab is inactive. To avoid flaky
// tests caused by the browser becoming inactive, we match on `::testing::_`.

class BraveAdsTabHelperTest : public PlatformBrowserTest {
 public:
  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    InitEmbeddedTestServer();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    PlatformBrowserTest::SetUpCommandLine(command_line);

    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    PlatformBrowserTest::SetUpInProcessBrowserTestFixture();

    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();

    callback_list_subscription_ =
        BrowserContextDependencyManager::GetInstance()
            ->RegisterCreateServicesCallbackForTesting(base::BindRepeating(
                &BraveAdsTabHelperTest::OnWillCreateBrowserContextServices,
                base::Unretained(this)));
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();

    PlatformBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  void OnWillCreateBrowserContextServices(
      content::BrowserContext* const context) {
    AdsServiceFactory::GetInstance()->SetTestingFactory(
        context, base::BindRepeating(&BraveAdsTabHelperTest::CreateAdsService,
                                     base::Unretained(this)));
  }

  std::unique_ptr<KeyedService> CreateAdsService(
      content::BrowserContext* const /*context*/) {
    // Since we are mocking the `AdsService`, a delegate is not required. Note
    // that we are not testing the `AdsService` itself, these tests are focused
    // on the `AdsTabHelper`.
    return std::make_unique<AdsServiceMock>(/*delegate*/ nullptr);
  }

  AdsServiceMock& GetAdsServiceMock() {
    AdsService* ads_service = AdsServiceFactory::GetForProfile(GetProfile());
    CHECK(ads_service);
    return *static_cast<AdsServiceMock*>(ads_service);
  }

  Profile* GetProfile() { return chrome_test_utils::GetProfile(this); }

  PrefService* GetPrefs() { return GetProfile()->GetPrefs(); }

  base::FilePath GetTestDataDir() {
    const base::ScopedAllowBlockingForTesting scoped_allow_blocking;
    return base::PathService::CheckedGet(brave::DIR_TEST_DATA);
  }

  void InitEmbeddedTestServer() {
    const base::FilePath test_data_dir = GetTestDataDir();

    test_server_.ServeFilesFromDirectory(test_data_dir);
    test_server_.RegisterRequestHandler(base::BindRepeating(
        &BraveAdsTabHelperTest::HandleRequest, base::Unretained(this)));
    test_server_handle_ = test_server_.StartAndReturnHandle();
    EXPECT_TRUE(test_server_handle_);
  }

  int32_t TabId() {
    content::WebContents* const web_contents = GetActiveWebContents();
    EXPECT_TRUE(web_contents);

    return sessions::SessionTabHelper::IdForTab(web_contents).id();
  }

  content::WebContents* GetActiveWebContents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  bool WaitForActiveWebContentsToLoad() {
    content::WebContents* const web_contents = GetActiveWebContents();
    EXPECT_TRUE(web_contents);

    web_contents->GetController().LoadIfNecessary();

    return content::WaitForLoadStop(web_contents);
  }

  void CloseActiveWebContents() {
    content::WebContents* const web_contents = GetActiveWebContents();
    chrome::CloseWebContents(browser(), web_contents, /*add_to_history=*/false);
  }

  void NavigateToURL(const std::string_view relative_url,
                     const bool has_user_gesture) {
    content::WebContents* const web_contents = GetActiveWebContents();
    EXPECT_TRUE(web_contents);

    const GURL url = test_server_.GetURL(kHostName, relative_url);

    content::NavigationController::LoadURLParams params(url);
    if (has_user_gesture) {
      return content::NavigateToURLBlockUntilNavigationsComplete(
          web_contents, url, /*number_of_navigations=*/1,
          /*ignore_uncommitted_navigations=*/true);
    }

    EXPECT_TRUE(NavigateToURLFromRendererWithoutUserGesture(web_contents, url));
  }

  void SimulateHttpStatusCodePage(const int http_status_code) {
    const std::string relative_url =
        absl::StrFormat("%s?%s=%d", kHandleRequestUrlPath,
                        kHttpStatusCodeQueryKey, http_status_code);
    NavigateToURL(relative_url, /*has_user_gesture=*/true);
  }

  ::testing::AssertionResult ExecuteJavaScript(const std::string& javascript,
                                               const bool has_user_gesture) {
    content::WebContents* const web_contents = GetActiveWebContents();
    return content::ExecJs(web_contents, javascript,
                           has_user_gesture
                               ? content::EXECUTE_SCRIPT_DEFAULT_OPTIONS
                               : content::EXECUTE_SCRIPT_NO_USER_GESTURE);
  }

  void GoBack() {
    content::WebContents* const web_contents = GetActiveWebContents();
    EXPECT_TRUE(web_contents);

    content::NavigationController& navigation_controller =
        web_contents->GetController();
    EXPECT_TRUE(navigation_controller.CanGoBack());
    navigation_controller.GoBack();
  }

  void GoForward() {
    content::WebContents* const web_contents = GetActiveWebContents();
    EXPECT_TRUE(web_contents);

    content::NavigationController& navigation_controller =
        web_contents->GetController();
    EXPECT_TRUE(navigation_controller.CanGoForward());
    navigation_controller.GoForward();
  }

  void Reload() {
    content::WebContents* const web_contents = GetActiveWebContents();
    EXPECT_TRUE(web_contents);

    web_contents->GetController().Reload(content::ReloadType::NORMAL,
                                         /*check_for_repost=*/false);
  }

  void SimulateClick(const std::string& selectors,
                     const bool has_user_gesture) {
    const std::string javascript = base::ReplaceStringPlaceholders(
        R"(document.querySelector("$1").click();)", {selectors}, nullptr);
    ExecuteJavaScript(javascript, has_user_gesture);
  }

  void StartVideoPlayback(const std::string& selectors) {
    const std::string javascript = base::ReplaceStringPlaceholders(
        R"(document.querySelector("$1")?.play();)", {selectors}, nullptr);

    // Video elements must be executed with a user gesture.
    ExecuteJavaScript(javascript, /*has_user_gesture=*/true);
  }

  void PauseVideoPlayback(const std::string& selectors) {
    const std::string javascript = base::ReplaceStringPlaceholders(
        R"(document.querySelector("$1")?.pause();)", {selectors}, nullptr);
    ExecuteJavaScript(javascript, /*has_user_gesture=*/true);
  }

  void RestoreBrowser(Profile* const profile) {
    CHECK(profile);

    SessionRestoreTestHelper session_restore_test_helper;
    chrome::OpenWindowWithRestoredTabs(profile);
    if (SessionRestore::IsRestoring(profile)) {
      session_restore_test_helper.Wait();
    }

    SelectFirstBrowser();
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleHttpStatusCodeQueryKey(
      const std::string& value) const {
    auto http_response =
        std::make_unique<net::test_server::BasicHttpResponse>();

    int http_status_code_as_int;
    EXPECT_TRUE(base::StringToInt(value, &http_status_code_as_int));
    const std::optional<net::HttpStatusCode> http_status_code =
        net::TryToGetHttpStatusCode(http_status_code_as_int);
    EXPECT_TRUE(http_status_code);
    http_response->set_code(*http_status_code);

    http_response->set_content_type("text/html");
    const std::string http_status_code_page = absl::StrFormat(
        R"(
            <html>
              <head>
                <title>
                  HTTP Status Code
                </title>
              </head>
              <body>
                %d (%s)
              </body>
            </html>)",
        *http_status_code, http_response->reason());
    http_response->set_content(http_status_code_page);

    return http_response;
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
      const net::test_server::HttpRequest& http_request) const {
    const GURL url = http_request.GetURL();
    if (url.path() != kHandleRequestUrlPath) {
      // Do not handle the request.
      return nullptr;
    }

    // Handle request.
    base::StringPairs key_value_pairs;
    base::SplitStringIntoKeyValuePairs(url.query(), '=', '&', &key_value_pairs);

    for (const auto& [key, value] : key_value_pairs) {
      if (key == kHttpStatusCodeQueryKey) {
        return HandleHttpStatusCodeQueryKey(value);
      }
    }

    NOTREACHED() << "Query key not found. Unable to handle the request.";
  }

  std::vector<GURL> RedirectChainExpectation(
      const std::string_view relative_url) const {
    const GURL url = test_server_.GetURL(kHostName, relative_url);
    return {url};
  }

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;

  base::CallbackListSubscription callback_list_subscription_;

  net::EmbeddedTestServer test_server_{
      net::test_server::EmbeddedTestServer::TYPE_HTTPS};
  net::test_server::EmbeddedTestServerHandle test_server_handle_;
};

IN_PROC_BROWSER_TEST_F(BraveAdsTabHelperTest, NotifyTabDidChange) {
  EXPECT_CALL(
      GetAdsServiceMock(),
      NotifyTabDidChange(TabId(),
                         RedirectChainExpectation(kMultiPageApplicationWebpage),
                         /*is_new_navigation=*/true, /*is_restoring=*/false,
                         /*is_visible=*/::testing::_))
      .Times(::testing::AtLeast(1));
  NavigateToURL(kMultiPageApplicationWebpage, /*has_user_gesture=*/true);
}

IN_PROC_BROWSER_TEST_F(BraveAdsTabHelperTest,
                       NotifyTabDidChangeIfTabWasRestored) {
  EXPECT_CALL(GetAdsServiceMock(), NotifyTabDidChange)
      .Times(::testing::AnyNumber());

  EXPECT_CALL(
      GetAdsServiceMock(),
      NotifyTabDidChange(TabId(),
                         RedirectChainExpectation(kMultiPageApplicationWebpage),
                         /*is_new_navigation=*/true, /*is_restoring=*/false,
                         /*is_visible=*/::testing::_))
      .Times(::testing::AtLeast(1));
  NavigateToURL(kMultiPageApplicationWebpage, /*has_user_gesture=*/true);

  // Must occur before the browser is closed.
  Profile* const profile = GetProfile();
  AdsServiceMock& ads_service_mock = GetAdsServiceMock();

  const ScopedKeepAlive scoped_keep_alive(KeepAliveOrigin::SESSION_RESTORE,
                                          KeepAliveRestartOption::DISABLED);
  const ScopedProfileKeepAlive scoped_profile_keep_alive(
      profile, ProfileKeepAliveOrigin::kSessionRestore);
  CloseBrowserSynchronously(browser());

  // We do not know the tab id until the tab is restored, so we match on
  // `::testing::_`.
  EXPECT_CALL(ads_service_mock,
              NotifyTabDidChange(
                  /*tab_id=*/::testing::_,
                  RedirectChainExpectation(kMultiPageApplicationWebpage),
                  /*is_new_navigation=*/false, /*is_restoring=*/true,
                  /*is_visible=*/::testing::_));
  RestoreBrowser(profile);

  EXPECT_TRUE(WaitForActiveWebContentsToLoad());
}

IN_PROC_BROWSER_TEST_F(BraveAdsTabHelperTest, NotifyTabDidLoad) {
  EXPECT_CALL(GetAdsServiceMock(), NotifyTabDidLoad(TabId(), net::HTTP_OK));
  NavigateToURL(kMultiPageApplicationWebpage, /*has_user_gesture=*/true);
}

IN_PROC_BROWSER_TEST_F(BraveAdsTabHelperTest,
                       NotifyTabDidLoadForHttpServerErrorResponsePage) {
  EXPECT_CALL(GetAdsServiceMock(),
              NotifyTabDidLoad(TabId(), net::HTTP_INTERNAL_SERVER_ERROR));
  SimulateHttpStatusCodePage(net::HTTP_INTERNAL_SERVER_ERROR);
}

IN_PROC_BROWSER_TEST_F(BraveAdsTabHelperTest,
                       NotifyTabDidLoadForHttpClientErrorResponsePage) {
  EXPECT_CALL(GetAdsServiceMock(),
              NotifyTabDidLoad(TabId(), net::HTTP_NOT_FOUND));
  SimulateHttpStatusCodePage(net::HTTP_NOT_FOUND);
}

IN_PROC_BROWSER_TEST_F(BraveAdsTabHelperTest,
                       NotifyTabDidLoadForHttpRedirectionResponsePage) {
  EXPECT_CALL(GetAdsServiceMock(),
              NotifyTabDidLoad(TabId(), net::HTTP_MOVED_PERMANENTLY));
  SimulateHttpStatusCodePage(net::HTTP_MOVED_PERMANENTLY);
}

IN_PROC_BROWSER_TEST_F(BraveAdsTabHelperTest,
                       NotifyTabDidLoadForHttpSuccessfulResponsePage) {
  EXPECT_CALL(GetAdsServiceMock(), NotifyTabDidLoad(TabId(), net::HTTP_OK));
  SimulateHttpStatusCodePage(net::HTTP_OK);
}

IN_PROC_BROWSER_TEST_F(BraveAdsTabHelperTest,
                       NotifyTabHtmlContentDidChangeForRewardsUser) {
  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);

  base::RunLoop run_loop;
  EXPECT_CALL(
      GetAdsServiceMock(),
      NotifyTabHtmlContentDidChange(
          TabId(), RedirectChainExpectation(kMultiPageApplicationWebpage),
          kMultiPageApplicationWebpageHtmlContent))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  NavigateToURL(kMultiPageApplicationWebpage, /*has_user_gesture=*/true);
  run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(
    BraveAdsTabHelperTest,
    NotifyTabHtmlContentDidChangeWithEmptyHtmlForNonRewardsUser) {
  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, false);

  base::RunLoop run_loop;
  EXPECT_CALL(
      GetAdsServiceMock(),
      NotifyTabHtmlContentDidChange(
          TabId(), RedirectChainExpectation(kMultiPageApplicationWebpage),
          /*html=*/::testing::IsEmpty()))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  NavigateToURL(kMultiPageApplicationWebpage, /*has_user_gesture=*/true);
  run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(BraveAdsTabHelperTest,
                       DoNotNotifyTabHtmlContentDidChangeIfTabWasRestored) {
  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);

  base::RunLoop run_loop;
  EXPECT_CALL(GetAdsServiceMock(), NotifyTabHtmlContentDidChange)
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  NavigateToURL(kMultiPageApplicationWebpage, /*has_user_gesture=*/true);
  run_loop.Run();
  ::testing::Mock::VerifyAndClearExpectations(&GetAdsServiceMock());

  // Must occur before the browser is closed.
  Profile* const profile = GetProfile();
  AdsServiceMock& ads_service_mock = GetAdsServiceMock();

  const ScopedKeepAlive scoped_keep_alive(KeepAliveOrigin::SESSION_RESTORE,
                                          KeepAliveRestartOption::DISABLED);
  const ScopedProfileKeepAlive scoped_profile_keep_alive(
      profile, ProfileKeepAliveOrigin::kSessionRestore);
  CloseBrowserSynchronously(browser());

  // We should not notify about changes to the tab's HTML content, as the
  // session will be restored and the tab will reload.
  EXPECT_CALL(ads_service_mock, NotifyTabHtmlContentDidChange).Times(0);
  RestoreBrowser(profile);

  EXPECT_TRUE(WaitForActiveWebContentsToLoad());
}

IN_PROC_BROWSER_TEST_F(
    BraveAdsTabHelperTest,
    DoNotNotifyTabHtmlContentDidChangeForPreviouslyCommittedNavigation) {
  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);

  base::RunLoop run_loop;
  EXPECT_CALL(GetAdsServiceMock(), NotifyTabHtmlContentDidChange)
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  NavigateToURL(kMultiPageApplicationWebpage, /*has_user_gesture=*/true);
  run_loop.Run();
  ::testing::Mock::VerifyAndClearExpectations(&GetAdsServiceMock());

  EXPECT_CALL(GetAdsServiceMock(), NotifyTabHtmlContentDidChange).Times(0);
  GoBack();
  GoForward();
  Reload();

  EXPECT_TRUE(WaitForActiveWebContentsToLoad());
}

IN_PROC_BROWSER_TEST_F(
    BraveAdsTabHelperTest,
    DoNotNotifyTabHtmlContentDidChangeForHttpClientErrorResponsePage) {
  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);

  EXPECT_CALL(GetAdsServiceMock(), NotifyTabHtmlContentDidChange).Times(0);
  SimulateHttpStatusCodePage(net::HTTP_NOT_FOUND);
}

IN_PROC_BROWSER_TEST_F(
    BraveAdsTabHelperTest,
    DoNotNotifyTabHtmlContentDidChangeForHttpServerErrorResponsePage) {
  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);

  EXPECT_CALL(GetAdsServiceMock(), NotifyTabHtmlContentDidChange).Times(0);
  SimulateHttpStatusCodePage(net::HTTP_INTERNAL_SERVER_ERROR);
}

IN_PROC_BROWSER_TEST_F(BraveAdsTabHelperTest,
                       NotifyTabHtmlContentDidChangeForSameDocumentNavigation) {
  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);

  {
    base::RunLoop run_loop;
    EXPECT_CALL(GetAdsServiceMock(), NotifyTabHtmlContentDidChange)
        .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
    NavigateToURL(kSinglePageApplicationWebpage, /*has_user_gesture=*/true);
    run_loop.Run();
    ::testing::Mock::VerifyAndClearExpectations(&GetAdsServiceMock());
  }

  {
    base::RunLoop run_loop;
    EXPECT_CALL(GetAdsServiceMock(),
                NotifyTabHtmlContentDidChange(
                    TabId(), ::testing::Contains(FileName("same_document")),
                    kSinglePageApplicationWebpageHtmlContent))
        .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
    SimulateClick(kSinglePageApplicationClickSelectors,
                  /*has_user_gesture=*/true);
    run_loop.Run();
  }
}

IN_PROC_BROWSER_TEST_F(
    BraveAdsTabHelperTest,
    NotifyTabTextContentDidChangeForRewardsUserOptedInToNotificationAds) {
  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);
  GetPrefs()->SetBoolean(prefs::kOptedInToNotificationAds, true);

  base::RunLoop run_loop;
  EXPECT_CALL(
      GetAdsServiceMock(),
      NotifyTabTextContentDidChange(
          TabId(), RedirectChainExpectation(kMultiPageApplicationWebpage),
          kMultiPageApplicationWebpageTextContent))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  NavigateToURL(kMultiPageApplicationWebpage, /*has_user_gesture=*/true);
  run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(BraveAdsTabHelperTest,
                       DoNotNotifyTabTextContentDidChangeForNonRewardsUser) {
  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, false);

  EXPECT_CALL(GetAdsServiceMock(), NotifyTabTextContentDidChange).Times(0);
  NavigateToURL(kMultiPageApplicationWebpage, /*has_user_gesture=*/true);
}

IN_PROC_BROWSER_TEST_F(
    BraveAdsTabHelperTest,
    DoNotNotifyTabTextContentDidChangeForNonRewardsUserAndOptedOutOfNotificationAds) {
  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, false);
  GetPrefs()->SetBoolean(prefs::kOptedInToNotificationAds, false);

  EXPECT_CALL(GetAdsServiceMock(), NotifyTabTextContentDidChange).Times(0);
  NavigateToURL(kMultiPageApplicationWebpage, /*has_user_gesture=*/true);
}

IN_PROC_BROWSER_TEST_F(
    BraveAdsTabHelperTest,
    DoNotNotifyTabTextContentDidChangeForRewardsUserOptedOutOfNotificationAds) {
  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);
  GetPrefs()->SetBoolean(prefs::kOptedInToNotificationAds, false);

  EXPECT_CALL(GetAdsServiceMock(), NotifyTabTextContentDidChange).Times(0);
  NavigateToURL(kMultiPageApplicationWebpage, /*has_user_gesture=*/true);
}

IN_PROC_BROWSER_TEST_F(BraveAdsTabHelperTest,
                       DoNotNotifyTabTextContentDidChangeIfTabWasRestored) {
  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);
  GetPrefs()->SetBoolean(prefs::kOptedInToNotificationAds, true);

  base::RunLoop run_loop;
  EXPECT_CALL(GetAdsServiceMock(), NotifyTabTextContentDidChange)
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  NavigateToURL(kMultiPageApplicationWebpage, /*has_user_gesture=*/true);
  run_loop.Run();
  ::testing::Mock::VerifyAndClearExpectations(&GetAdsServiceMock());

  // Must occur before the browser is closed.
  Profile* const profile = GetProfile();
  AdsServiceMock& ads_service_mock = GetAdsServiceMock();

  const ScopedKeepAlive scoped_keep_alive(KeepAliveOrigin::SESSION_RESTORE,
                                          KeepAliveRestartOption::DISABLED);
  const ScopedProfileKeepAlive scoped_profile_keep_alive(
      profile, ProfileKeepAliveOrigin::kSessionRestore);
  CloseBrowserSynchronously(browser());

  // We should not notify about changes to the tab's text content, as the
  // session will be restored and the tab will reload.
  EXPECT_CALL(ads_service_mock, NotifyTabTextContentDidChange).Times(0);
  RestoreBrowser(profile);

  EXPECT_TRUE(WaitForActiveWebContentsToLoad());
}

IN_PROC_BROWSER_TEST_F(
    BraveAdsTabHelperTest,
    DoNotNotifyTabTextContentDidChangeForPreviouslyCommittedNavigation) {
  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);
  GetPrefs()->SetBoolean(prefs::kOptedInToNotificationAds, true);

  base::RunLoop run_loop;
  EXPECT_CALL(GetAdsServiceMock(), NotifyTabTextContentDidChange)
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  NavigateToURL(kMultiPageApplicationWebpage, /*has_user_gesture=*/true);
  run_loop.Run();
  ::testing::Mock::VerifyAndClearExpectations(&GetAdsServiceMock());

  EXPECT_CALL(GetAdsServiceMock(), NotifyTabTextContentDidChange).Times(0);
  GoBack();
  GoForward();
  Reload();

  EXPECT_TRUE(WaitForActiveWebContentsToLoad());
}

IN_PROC_BROWSER_TEST_F(
    BraveAdsTabHelperTest,
    DoNotNotifyTabTextContentDidChangeForHttpClientErrorResponsePage) {
  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);
  GetPrefs()->SetBoolean(prefs::kOptedInToNotificationAds, true);

  EXPECT_CALL(GetAdsServiceMock(), NotifyTabTextContentDidChange).Times(0);
  SimulateHttpStatusCodePage(net::HTTP_NOT_FOUND);
}

IN_PROC_BROWSER_TEST_F(
    BraveAdsTabHelperTest,
    DoNotNotifyTabTextContentDidChangeForHttpServerErrorResponsePage) {
  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);
  GetPrefs()->SetBoolean(prefs::kOptedInToNotificationAds, true);

  EXPECT_CALL(GetAdsServiceMock(), NotifyTabTextContentDidChange).Times(0);
  SimulateHttpStatusCodePage(net::HTTP_INTERNAL_SERVER_ERROR);
}

IN_PROC_BROWSER_TEST_F(
    BraveAdsTabHelperTest,
    DoNotNotifyTabTextContentDidChangeForSameDocumentNavigation) {
  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);
  GetPrefs()->SetBoolean(prefs::kOptedInToNotificationAds, true);

  base::RunLoop run_loop;
  EXPECT_CALL(GetAdsServiceMock(), NotifyTabTextContentDidChange)
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  NavigateToURL(kSinglePageApplicationWebpage, /*has_user_gesture=*/true);
  run_loop.Run();
  ::testing::Mock::VerifyAndClearExpectations(&GetAdsServiceMock());

  EXPECT_CALL(GetAdsServiceMock(), NotifyTabTextContentDidChange).Times(0);
  SimulateClick(kSinglePageApplicationClickSelectors,
                /*has_user_gesture=*/true);

  EXPECT_TRUE(WaitForActiveWebContentsToLoad());
}

IN_PROC_BROWSER_TEST_F(BraveAdsTabHelperTest,
                       NotifyTabDidStartPlayingMediaForAutoplayVideo) {
  GetPrefs()->SetBoolean(::prefs::kAutoplayAllowed, true);

  content::WebContents* const web_contents = GetActiveWebContents();
  MediaWaiter waiter(web_contents);

  EXPECT_CALL(GetAdsServiceMock(), NotifyTabDidStartPlayingMedia);
  NavigateToURL(kAutoplayVideoWebpage, /*has_user_gesture=*/true);

  waiter.WaitForMediaStartedPlaying();
}

IN_PROC_BROWSER_TEST_F(
    BraveAdsTabHelperTest,
    DoNotNotifyTabDidStartPlayingMediaForAutoplayVideoIfDisallowed) {
  GetPrefs()->SetBoolean(::prefs::kAutoplayAllowed, false);

  content::WebContents* const web_contents = GetActiveWebContents();
  MediaWaiter waiter(web_contents);

  EXPECT_CALL(GetAdsServiceMock(), NotifyTabDidStartPlayingMedia).Times(0);
  NavigateToURL(kAutoplayVideoWebpage, /*has_user_gesture=*/true);

  waiter.WaitForMediaSessionCreated();
}

IN_PROC_BROWSER_TEST_F(BraveAdsTabHelperTest,
                       NotifyTabDidStopPlayingMediaForAutoplayVideo) {
  GetPrefs()->SetBoolean(::prefs::kAutoplayAllowed, true);

  content::WebContents* const web_contents = GetActiveWebContents();
  MediaWaiter waiter(web_contents);

  EXPECT_CALL(GetAdsServiceMock(), NotifyTabDidStartPlayingMedia);
  NavigateToURL(kAutoplayVideoWebpage, /*has_user_gesture=*/true);

  waiter.WaitForMediaStartedPlaying();

  EXPECT_CALL(GetAdsServiceMock(), NotifyTabDidStopPlayingMedia);
  PauseVideoPlayback(kVideoJavascriptDocumentQuerySelectors);
}

IN_PROC_BROWSER_TEST_F(BraveAdsTabHelperTest, NotifyTabDidStartPlayingMedia) {
  NavigateToURL(kVideoWebpage, /*has_user_gesture=*/true);

  EXPECT_CALL(GetAdsServiceMock(), NotifyTabDidStartPlayingMedia);
  StartVideoPlayback(kVideoJavascriptDocumentQuerySelectors);
}

IN_PROC_BROWSER_TEST_F(BraveAdsTabHelperTest, NotifyTabDidStopPlayingMedia) {
  NavigateToURL(kVideoWebpage, /*has_user_gesture=*/true);

  StartVideoPlayback(kVideoJavascriptDocumentQuerySelectors);

  EXPECT_CALL(GetAdsServiceMock(), NotifyTabDidStopPlayingMedia);
  PauseVideoPlayback(kVideoJavascriptDocumentQuerySelectors);
}

IN_PROC_BROWSER_TEST_F(BraveAdsTabHelperTest, NotifyDidCloseTab) {
  EXPECT_CALL(GetAdsServiceMock(), NotifyDidCloseTab);
  CloseActiveWebContents();
}

IN_PROC_BROWSER_TEST_F(BraveAdsTabHelperTest, NotifyUserGestureEventTriggered) {
  EXPECT_CALL(GetAdsServiceMock(), NotifyUserGestureEventTriggered)
      .Times(::testing::AtLeast(1));
  NavigateToURL(kMultiPageApplicationWebpage, /*has_user_gesture=*/true);
}

IN_PROC_BROWSER_TEST_F(BraveAdsTabHelperTest,
                       DoNotNotifyUserGestureEventTriggered) {
  EXPECT_CALL(GetAdsServiceMock(), NotifyUserGestureEventTriggered).Times(0);
  NavigateToURL(kMultiPageApplicationWebpage, /*has_user_gesture=*/false);
}

IN_PROC_BROWSER_TEST_F(BraveAdsTabHelperTest,
                       DoNotNotifyUserGestureEventTriggeredIfTabWasRestored) {
  EXPECT_CALL(GetAdsServiceMock(), NotifyUserGestureEventTriggered)
      .Times(::testing::AtLeast(1));
  NavigateToURL(kMultiPageApplicationWebpage, /*has_user_gesture=*/true);
  ::testing::Mock::VerifyAndClearExpectations(&GetAdsServiceMock());

  // Must occur before the browser is closed.
  Profile* const profile = GetProfile();
  AdsServiceMock& ads_service_mock = GetAdsServiceMock();

  const ScopedKeepAlive scoped_keep_alive(KeepAliveOrigin::SESSION_RESTORE,
                                          KeepAliveRestartOption::DISABLED);
  const ScopedProfileKeepAlive scoped_profile_keep_alive(
      profile, ProfileKeepAliveOrigin::kSessionRestore);
  CloseBrowserSynchronously(browser());

  EXPECT_CALL(ads_service_mock, NotifyUserGestureEventTriggered).Times(0);
  RestoreBrowser(profile);

  EXPECT_TRUE(WaitForActiveWebContentsToLoad());
}

IN_PROC_BROWSER_TEST_F(BraveAdsTabHelperTest,
                       CreativeAdsServiceForRegularBrowser) {
  content::WebContents* const web_contents = GetActiveWebContents();
  ASSERT_TRUE(web_contents);

  AdsTabHelper* const ads_tab_helper =
      AdsTabHelper::FromWebContents(web_contents);
  ASSERT_TRUE(ads_tab_helper);

  EXPECT_TRUE(ads_tab_helper->ads_service());
}

IN_PROC_BROWSER_TEST_F(BraveAdsTabHelperTest,
                       DoNotCreativeAdsServiceForIncognitoBrowser) {
  const Browser* const browser = CreateIncognitoBrowser();

  content::WebContents* const web_contents =
      browser->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents);

  AdsTabHelper* const ads_tab_helper =
      AdsTabHelper::FromWebContents(web_contents);
  ASSERT_TRUE(ads_tab_helper);

  EXPECT_FALSE(ads_tab_helper->ads_service());
}

IN_PROC_BROWSER_TEST_F(BraveAdsTabHelperTest,
                       DoNotCreativeAdsServiceForGuestBrowser) {
  const Browser* const browser = CreateGuestBrowser();

  content::WebContents* const web_contents =
      browser->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents);

  AdsTabHelper* const ads_tab_helper =
      AdsTabHelper::FromWebContents(web_contents);
  ASSERT_TRUE(ads_tab_helper);

  EXPECT_FALSE(ads_tab_helper->ads_service());
}

}  // namespace brave_ads
