/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

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
#include "base/functional/callback_helpers.h"
#include "base/test/gmock_callback_support.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/notifications/ads_notification_handler.h"
#include "brave/components/brave_ads/core/browser/service/ads_service_mock.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/core/pref_names.h"
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
#include "third_party/abseil-cpp/absl/strings/str_format.h"
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

constexpr char kVideoWebpage[] = "/brave_ads/video.html";
constexpr char kVideoJavascriptDocumentQuerySelectors[] = "video";

constexpr char kTestPlacementId[] = "a68e4727-5ce7-4a68-a30f-6ddc882b6e65";

std::unique_ptr<net::test_server::HttpResponse> HandleHttpStatusCodeQueryKey(
    const std::string& value) {
  auto http_response = std::make_unique<net::test_server::BasicHttpResponse>();

  int http_status_code_as_int;
  EXPECT_TRUE(base::StringToInt(value, &http_status_code_as_int));
  std::optional<net::HttpStatusCode> http_status_code =
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
    const net::test_server::HttpRequest& http_request) {
  const GURL url = http_request.GetURL();
  if (url.path() != kHandleRequestUrlPath) {
    return nullptr;
  }

  base::StringPairs key_value_pairs;
  base::SplitStringIntoKeyValuePairs(url.query(), '=', '&', &key_value_pairs);

  for (const auto& [key, value] : key_value_pairs) {
    if (key == kHttpStatusCodeQueryKey) {
      return HandleHttpStatusCodeQueryKey(value);
    }
  }

  NOTREACHED() << "Query key not found. Unable to handle the request.";
}

base::FilePath GetTestDataDir() {
  const base::ScopedAllowBlockingForTesting scoped_allow_blocking;
  return base::PathService::CheckedGet(brave::DIR_TEST_DATA);
}

}  // namespace

class MediaWaiter final : public content::WebContentsObserver {
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

class BraveAdsServiceTest : public PlatformBrowserTest {
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
                &BraveAdsServiceTest::OnWillCreateBrowserContextServices,
                base::Unretained(this)));
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();

    PlatformBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  void OnWillCreateBrowserContextServices(
      content::BrowserContext* const context) {
    AdsServiceFactory::GetInstance()->SetTestingFactory(
        context, base::BindRepeating(&BraveAdsServiceTest::CreateAdsService,
                                     base::Unretained(this)));
  }

  std::unique_ptr<KeyedService> CreateAdsService(
      content::BrowserContext* const /*context*/) {
    // Since we are mocking the `AdsService`, a delegate is not required. Note
    // that we are not testing the `AdsService` itself, these tests are focused
    // on the `AdsTabHelper`.
    return std::make_unique<AdsServiceMock>();
  }

  AdsServiceMock& GetAdsServiceMock() {
    AdsService* ads_service = AdsServiceFactory::GetForProfile(GetProfile());
    CHECK(ads_service);
    return *static_cast<AdsServiceMock*>(ads_service);
  }

  Profile* GetProfile() { return chrome_test_utils::GetProfile(this); }

  PrefService* GetPrefs() { return GetProfile()->GetPrefs(); }

  void InitEmbeddedTestServer() {
    const base::FilePath test_data_dir = GetTestDataDir();

    test_server_.ServeFilesFromDirectory(test_data_dir);
    test_server_.RegisterRequestHandler(base::BindRepeating(&HandleRequest));
    test_server_handle_ = test_server_.StartAndReturnHandle();
    EXPECT_TRUE(test_server_handle_);
  }

  content::WebContents* GetActiveWebContents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  int32_t TabId() {
    content::WebContents* const web_contents = GetActiveWebContents();
    EXPECT_TRUE(web_contents);

    return sessions::SessionTabHelper::IdForTab(web_contents).id();
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

  void NavigateToRelativeURL(std::string_view relative_url,
                             bool has_user_gesture) {
    content::WebContents* const web_contents = GetActiveWebContents();
    EXPECT_TRUE(web_contents);

    const GURL url = test_server_.GetURL(kHostName, relative_url);

    if (has_user_gesture) {
      return content::NavigateToURLBlockUntilNavigationsComplete(
          web_contents, url, /*number_of_navigations=*/1,
          /*ignore_uncommitted_navigations=*/true);
    }

    EXPECT_TRUE(NavigateToURLFromRendererWithoutUserGesture(web_contents, url));
  }

  ::testing::AssertionResult ExecuteJavaScript(const std::string& javascript,
                                               bool has_user_gesture) {
    content::WebContents* const web_contents = GetActiveWebContents();
    return content::ExecJs(web_contents, javascript,
                           has_user_gesture
                               ? content::EXECUTE_SCRIPT_DEFAULT_OPTIONS
                               : content::EXECUTE_SCRIPT_NO_USER_GESTURE);
  }

  void StartVideoPlayback(const std::string& selector) {
    const std::string javascript = base::ReplaceStringPlaceholders(
        R"(document.querySelector("$1")?.play();)", {selector}, nullptr);

    // Video elements must be executed with a user gesture.
    ASSERT_TRUE(ExecuteJavaScript(javascript, /*has_user_gesture=*/true));
  }

  void PauseVideoPlayback(const std::string& selector) {
    const std::string javascript = base::ReplaceStringPlaceholders(
        R"(document.querySelector("$1")?.pause();)", {selector}, nullptr);
    ASSERT_TRUE(ExecuteJavaScript(javascript, /*has_user_gesture=*/true));
  }

  std::vector<GURL> RedirectChainExpectation(
      std::string_view relative_url) const {
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

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, OnNotificationAdShown) {
  // AdsNotificationHandler::OnShow is the production entry point called by the
  // notification system when a notification ad becomes visible.
  EXPECT_CALL(GetAdsServiceMock(), OnNotificationAdShown(kTestPlacementId));

  AdsNotificationHandler ads_notification_handler(*GetProfile());
  ads_notification_handler.OnShow(GetProfile(), kTestPlacementId);
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, OnNotificationAdClosedIfDismissed) {
  // AdsNotificationHandler::OnClose with by_user=true is the production path
  // when the user manually dismisses a notification ad.
  EXPECT_CALL(GetAdsServiceMock(),
              OnNotificationAdClosed(kTestPlacementId, /*by_user=*/true));

  AdsNotificationHandler ads_notification_handler(*GetProfile());
  ads_notification_handler.OnClose(GetProfile(), GURL(), kTestPlacementId, /*by_user=*/true,
                  base::DoNothing());
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, OnNotificationAdClosedIfTimedOut) {
  // AdsNotificationHandler::OnClose with by_user=false is the production path
  // when a notification ad expires without user interaction.
  EXPECT_CALL(GetAdsServiceMock(),
              OnNotificationAdClosed(kTestPlacementId, /*by_user=*/false));

  AdsNotificationHandler ads_notification_handler(*GetProfile());
  ads_notification_handler.OnClose(GetProfile(), GURL(), kTestPlacementId, /*by_user=*/false,
                  base::DoNothing());
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, OnNotificationAdClicked) {
  // AdsNotificationHandler::OnClick is the production path called when the
  // user clicks a notification ad.
  EXPECT_CALL(GetAdsServiceMock(), OnNotificationAdClicked(kTestPlacementId));

  AdsNotificationHandler ads_notification_handler(*GetProfile());
  ads_notification_handler.OnClick(GetProfile(), GURL(), kTestPlacementId,
                  /*action_index=*/std::nullopt, /*reply=*/std::nullopt,
                  base::DoNothing());
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, NotifyTabTextContentDidChange) {
  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);
  GetPrefs()->SetBoolean(prefs::kOptedInToNotificationAds, true);

  base::RunLoop run_loop;
  EXPECT_CALL(
      GetAdsServiceMock(),
      NotifyTabTextContentDidChange(
          TabId(), RedirectChainExpectation(kMultiPageApplicationWebpage),
          kMultiPageApplicationWebpageTextContent))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  NavigateToRelativeURL(kMultiPageApplicationWebpage,
                        /*has_user_gesture=*/true);
  run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, NotifyTabHtmlContentDidChange) {
  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);

  base::RunLoop run_loop;
  EXPECT_CALL(
      GetAdsServiceMock(),
      NotifyTabHtmlContentDidChange(
          TabId(), RedirectChainExpectation(kMultiPageApplicationWebpage),
          kMultiPageApplicationWebpageHtmlContent))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  NavigateToRelativeURL(kMultiPageApplicationWebpage,
                        /*has_user_gesture=*/true);
  run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, NotifyTabDidStartPlayingMedia) {
  NavigateToRelativeURL(kVideoWebpage, /*has_user_gesture=*/true);

  EXPECT_CALL(GetAdsServiceMock(), NotifyTabDidStartPlayingMedia);
  StartVideoPlayback(kVideoJavascriptDocumentQuerySelectors);
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, NotifyTabDidStopPlayingMedia) {
  NavigateToRelativeURL(kVideoWebpage, /*has_user_gesture=*/true);
  StartVideoPlayback(kVideoJavascriptDocumentQuerySelectors);

  EXPECT_CALL(GetAdsServiceMock(), NotifyTabDidStopPlayingMedia);
  PauseVideoPlayback(kVideoJavascriptDocumentQuerySelectors);
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, NotifyTabDidChange) {
  EXPECT_CALL(
      GetAdsServiceMock(),
      NotifyTabDidChange(TabId(),
                         RedirectChainExpectation(kMultiPageApplicationWebpage),
                         /*is_new_navigation=*/true, /*is_restoring=*/false,
                         /*is_visible=*/::testing::_))
      .Times(::testing::AtLeast(1));
  NavigateToRelativeURL(kMultiPageApplicationWebpage,
                        /*has_user_gesture=*/true);
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, NotifyTabDidLoad) {
  EXPECT_CALL(GetAdsServiceMock(), NotifyTabDidLoad(TabId(), net::HTTP_OK));
  NavigateToRelativeURL(kMultiPageApplicationWebpage,
                        /*has_user_gesture=*/true);
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, NotifyDidCloseTab) {
  EXPECT_CALL(GetAdsServiceMock(), NotifyDidCloseTab);
  CloseActiveWebContents();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, NotifyUserGestureEventTriggered) {
  EXPECT_CALL(GetAdsServiceMock(), NotifyUserGestureEventTriggered)
      .Times(::testing::AtLeast(1));
  NavigateToRelativeURL(kMultiPageApplicationWebpage,
                        /*has_user_gesture=*/true);
}

#if !BUILDFLAG(IS_ANDROID)
IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, NotifyBrowserDidResignActive) {
  base::RunLoop run_loop;
  EXPECT_CALL(GetAdsServiceMock(), NotifyBrowserDidResignActive())
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));

  // Creating a second browser window causes the first to resign active.
  // AdsTabHelper observes BrowserList and calls NotifyBrowserDidResignActive
  // on the service when OnBrowserNoLongerActive fires.
  Browser* const second_browser = CreateBrowser(GetProfile());
  run_loop.Run();

  CloseBrowserSynchronously(second_browser);
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, NotifyBrowserDidBecomeActive) {
  EXPECT_CALL(GetAdsServiceMock(), NotifyBrowserDidResignActive())
      .Times(::testing::AnyNumber());
  Browser* const second_browser = CreateBrowser(GetProfile());
  ::testing::Mock::VerifyAndClearExpectations(&GetAdsServiceMock());

  base::RunLoop run_loop;
  EXPECT_CALL(GetAdsServiceMock(), NotifyBrowserDidBecomeActive())
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  // Closing the second browser restores focus to the first, which causes
  // AdsTabHelper to call NotifyBrowserDidBecomeActive on the service.
  CloseBrowserSynchronously(second_browser);
  run_loop.Run();
}
#endif  // !BUILDFLAG(IS_ANDROID)

}  // namespace brave_ads
