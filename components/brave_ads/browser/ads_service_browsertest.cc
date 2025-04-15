/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/ads_service.h"

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

}  // namespace

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

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;

  base::CallbackListSubscription callback_list_subscription_;

  net::EmbeddedTestServer test_server_{
      net::test_server::EmbeddedTestServer::TYPE_HTTPS};
  net::test_server::EmbeddedTestServerHandle test_server_handle_;
};

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest,
                       IsBrowserUpgradeRequiredToServeAds) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest,
                       IsBrowserUpgradeNotRequiredToServeAds) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, GetMaximumNotificationAdsPerHour) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest,
                       GetDefaultMaximumNotificationAdsPerHour) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, OnNotificationAdShown) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, OnNotificationAdClosedIfDismissed) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, OnNotificationAdClosedIfTimedOut) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, OnNotificationAdClicked) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, ClearData) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, GetDiagnostics) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, GetStatementOfAccounts) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, MaybeServeInlineContentAd) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, TriggerInlineContentAdEvent) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, PrefetchNewTabPageAd) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest,
                       DoNotPrefetchNewTabPageAdIfAlreadyPrefetched) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest,
                       DoNotPrefetchNewTabPageAdIfAlreadyPrefetching) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest,
                       GetPrefetchedNewTabPageAdForDisplay) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(
    BraveAdsServiceTest,
    DoNotGetPrefetchedNewTabPageAdForDisplayIfNotPrefetched) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, OnFailedToPrefetchNewTabPageAd) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, TriggerNewTabPageAdEvent) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, TriggerPromotedContentAdEvent) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, MaybeGetSearchResultAd) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, TriggerSearchResultAdEvent) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, PurgeOrphanedAdEventsForType) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, GetAdHistory) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, ToggleLikeAd) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, ToggleDislikeAd) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, ToggleLikeSegment) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, ToggleDislikeSegment) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, ToggleSaveAd) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, ToggleMarkAdAsInappropriate) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, NotifyTabTextContentDidChange) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, NotifyTabHtmlContentDidChange) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, NotifyTabDidStartPlayingMedia) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, NotifyTabDidStopPlayingMedia) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, NotifyTabDidChange) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, NotifyTabDidLoad) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, NotifyDidCloseTab) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, NotifyUserGestureEventTriggered) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, NotifyBrowserDidBecomeActive) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, NotifyBrowserDidResignActive) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, NotifyDidSolveAdaptiveCaptcha) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceTest, NotifyDidSolveAdaptiveCaptcha) {
  FAIL();
}

}  // namespace brave_ads
