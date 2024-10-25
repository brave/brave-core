// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <string>
#include <string_view>
#include <tuple>

#include "base/memory/raw_ref.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/timer/timer.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/brave_rewards/rewards_tab_helper.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/common/features.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

using base::test::TestFuture;
using net::test_server::BasicHttpResponse;
using net::test_server::HttpRequest;
using net::test_server::HttpResponse;

namespace brave_rewards {

class CreatorDetectionBrowserTest : public PlatformBrowserTest {
 protected:
  CreatorDetectionBrowserTest() {
    scoped_feature_list_.InitWithFeatures(
        {features::kPlatformCreatorDetectionFeature}, {});
  }

  ~CreatorDetectionBrowserTest() override = default;

  void DisableFeature() {
    scoped_feature_list_.Reset();
    scoped_feature_list_.InitWithFeatures(
        {}, {features::kPlatformCreatorDetectionFeature});
  }

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");
    embedded_https_test_server().SetCertHostnames(
        {"twitter.com", "github.com", "api.github.com", "reddit.com",
         "www.twitch.tv", "vimeo.com", "www.youtube.com", "abc.youtube.com",
         "example-creator.com"});
    embedded_https_test_server().RegisterRequestHandler(base::BindRepeating(
        &CreatorDetectionBrowserTest::HandleRequest, base::Unretained(this)));
    CHECK(embedded_https_test_server().Start());
  }

  void TearDownOnMainThread() override {
    EXPECT_TRUE(embedded_https_test_server().ShutdownAndWaitUntilComplete());
    PlatformBrowserTest::TearDownOnMainThread();
  }

  using RequestCallback =
      base::RepeatingCallback<void(std::string_view, BasicHttpResponse&)>;

  template <typename F>
  void SetRequestCallback(F callback) {
    request_callback_ = base::BindLambdaForTesting(callback);
  }

  void SetResponseHTML(std::string_view html) {
    std::string content(html);
    SetRequestCallback(
        [content](std::string_view path, BasicHttpResponse& response) {
          response.set_content(content);
        });
  }

  bool NavigateTo(std::string_view host, std::string_view path) {
    auto url = embedded_https_test_server().GetURL(host, path);
    return ui_test_utils::NavigateToURL(browser(), url);
  }

  RewardsTabHelper& GetRewardsTabHelper() {
    auto* web_contents = chrome_test_utils::GetActiveWebContents(this);
    CHECK(web_contents);
    auto* tab_helper = RewardsTabHelper::FromWebContents(web_contents);
    CHECK(tab_helper);
    return *tab_helper;
  }

  void WaitForTimeout() {
    base::RunLoop run_loop;
    base::OneShotTimer timeout;
    timeout.Start(FROM_HERE, base::Seconds(2), run_loop.QuitClosure());
    run_loop.Run();
  }

  RewardsService& GetRewardsService() {
    auto* rewards_service =
        RewardsServiceFactory::GetForProfile(browser()->profile());
    CHECK(rewards_service);
    return *rewards_service;
  }

  void EnableRewards() {
    auto* prefs = browser()->profile()->GetPrefs();
    prefs->SetBoolean(brave_rewards::prefs::kEnabled, true);

    TestFuture<mojom::CreateRewardsWalletResult> future;
    GetRewardsService().CreateRewardsWallet("US", future.GetCallback());
    CHECK(future.Wait());
  }

  mojom::PublisherInfoPtr WaitForPublisherInfo(const std::string& id) {
    for (;;) {
      TestFuture<mojom::Result, mojom::PublisherInfoPtr> future;
      GetRewardsService().GetPublisherInfo(id, future.GetCallback());
      auto info = std::get<1>(future.Take());
      if (info) {
        return info;
      }
      WaitForTimeout();
    }
  }

  class TabHelperObserver : public RewardsTabHelper::Observer {
   public:
    TabHelperObserver(RewardsTabHelper& tab_helper,
                      base::OnceCallback<void(std::string)> callback)
        : callback_(std::move(callback)) {
      tab_helper_observation_.Observe(&tab_helper);
    }

    ~TabHelperObserver() override = default;

    void OnPublisherForTabUpdated(const std::string& publisher_id) override {
      if (callback_) {
        std::move(callback_).Run(publisher_id);
      }
    }

   private:
    base::OnceCallback<void(std::string)> callback_;
    RewardsTabHelper::Observation tab_helper_observation_{this};
  };

 private:
  std::unique_ptr<HttpResponse> HandleRequest(const HttpRequest& request) {
    auto response = std::make_unique<BasicHttpResponse>();
    response->set_code(net::HTTP_OK);
    response->set_content_type("text/html;charset=utf-8");
    if (request_callback_) {
      request_callback_.Run(request.relative_url, *response);
    }
    return response;
  }

  base::test::ScopedFeatureList scoped_feature_list_;
  RequestCallback request_callback_;
  std::string response_html_;
};

IN_PROC_BROWSER_TEST_F(CreatorDetectionBrowserTest, NonPlatformSite) {
  EnableRewards();

  SetResponseHTML(R"(
    <!doctype html>
    <html>
      <body>Example creator</body>
    </html>
  )");

  AddBlankTabAndShow(browser());
  TestFuture<std::string> id_future;
  TabHelperObserver observer(GetRewardsTabHelper(), id_future.GetCallback());
  ASSERT_TRUE(NavigateTo("example-creator.com", "/"));
  EXPECT_EQ(id_future.Get(), "example-creator.com");
  EXPECT_TRUE(WaitForPublisherInfo("example-creator.com"));
}

IN_PROC_BROWSER_TEST_F(CreatorDetectionBrowserTest, GithubDetection) {
  EnableRewards();

  SetRequestCallback([](std::string_view path, BasicHttpResponse& response) {
    if (path == "/users/testuser") {
      response.AddCustomHeader("Access-Control-Allow-Origin", "*");
      response.set_content_type("application/json");
      response.set_content(R"(
          {"id": "1234567",
           "avatar_url": "https://github.com/user-avatar"} )");
      return;
    }
  });

  AddBlankTabAndShow(browser());
  TestFuture<std::string> future;
  TabHelperObserver observer(GetRewardsTabHelper(), future.GetCallback());
  ASSERT_TRUE(NavigateTo("github.com", "/testuser"));
  EXPECT_EQ(future.Get(), "github#channel:1234567");
}

IN_PROC_BROWSER_TEST_F(CreatorDetectionBrowserTest, RedditDetection) {
  EnableRewards();

  SetRequestCallback([](std::string_view path, BasicHttpResponse& response) {
    if (path == "/user/testuser/about.json") {
      response.set_content_type("application/json");
      response.set_content(R"(
          {"kind": "t2",
           "data": {
             "id": "987654321",
             "icon_img": "https://reddit.com/user-avatar"}} )");
      return;
    }
  });

  AddBlankTabAndShow(browser());
  TestFuture<std::string> future;
  TabHelperObserver observer(GetRewardsTabHelper(), future.GetCallback());
  ASSERT_TRUE(NavigateTo("reddit.com", "/user/testuser"));

  EXPECT_EQ(future.Get(), "reddit#channel:987654321");
}

IN_PROC_BROWSER_TEST_F(CreatorDetectionBrowserTest, TwitchDetection) {
  EnableRewards();

  SetResponseHTML(R"(
    <!doctype html>
    <html>
    <body>
      <h1 class="tw-title">Name</h1>
      <div class="channel-info-content">
        <div class="tw-avatar">
          <img src="/user-avatar" />
        </div>
      </div>
    </body>
    </html>
  )");

  AddBlankTabAndShow(browser());
  TestFuture<std::string> future;
  TabHelperObserver observer(GetRewardsTabHelper(), future.GetCallback());
  ASSERT_TRUE(NavigateTo("www.twitch.tv", "/testuser"));

  EXPECT_EQ(future.Get(), "twitch#author:testuser");
}

IN_PROC_BROWSER_TEST_F(CreatorDetectionBrowserTest, TwitterDetection) {
  EnableRewards();

  SetResponseHTML(R"(
    <!doctype html>
    <html>
    <head>
      <script>
        addEventListener('load', () => {
          const userEntities = {
            '987654321': {
              screen_name: 'testuser',
              profile_image_url_https: 'https://twitter.com/img'
            }
          }

          function getState() {
            return { entities: { users: { entities: userEntities } } }
          }

          Object.assign(document.querySelector('#react-root > div'), {
            __reactProps$123: { children: { props: { store: { getState } } } }
          })
        })
      </script>
    </head>
    <body>
      <div id="react-root">
        <div></div>
      </div>
    </body>
    </html>
  )");

  AddBlankTabAndShow(browser());
  TestFuture<std::string> future;
  TabHelperObserver observer(GetRewardsTabHelper(), future.GetCallback());
  ASSERT_TRUE(NavigateTo("twitter.com", "/testuser"));

  EXPECT_EQ(future.Get(), "twitter#channel:987654321");
}

IN_PROC_BROWSER_TEST_F(CreatorDetectionBrowserTest, VimeoDetection) {
  EnableRewards();

  SetResponseHTML(R"(
    <!doctype html>
    <html>
    <head>
      <script type="application/ld+json">
        [
          {
            "@type": "Person",
            "identifier": "987654321",
            "name": "Test User",
            "url": "https://vimeo.com/testuser",
            "image": "https://vimeo.com/user-avatar"
          }
        ]
      </script>
    </head>
    <body>
    </body>
    </html>
  )");

  AddBlankTabAndShow(browser());
  TestFuture<std::string> future;
  TabHelperObserver observer(GetRewardsTabHelper(), future.GetCallback());
  ASSERT_TRUE(NavigateTo("vimeo.com", "/testuser"));

  EXPECT_EQ(future.Get(), "vimeo#channel:987654321");
}

constexpr std::string_view kYouTubeHTML = R"(
  <!doctype html>
  <html>
  <head>
    <link rel="canonical" href="/channel/987654321" />
    <script>
      function triggerSameDocNav() {
        history.pushState(null, '', '/@testuser2')
        document.querySelector('#test-owner-link').href = '/@testuser2'
        document.querySelector('#avatar img').src = '/user-avatar-2'
        document.querySelector('.ytp-ce-channel-title').href =
          '/channel/123456789'
      }
    </script>
  </head>
  <body>
    <ytd-video-owner-renderer>
      <a id="test-owner-link" href="/@testuser"></a>
    </ytd-video-owner-renderer>
    <div id="avatar">
      <img id="test-image" src="/user-avatar" />
    </div>
    <a class='ytp-ce-channel-title'></a>
  </body>
  </html>
)";

IN_PROC_BROWSER_TEST_F(CreatorDetectionBrowserTest, YouTubeDetection) {
  EnableRewards();
  SetResponseHTML(kYouTubeHTML);

  AddBlankTabAndShow(browser());

  {
    // Initial page load.
    TestFuture<std::string> future;
    TabHelperObserver observer(GetRewardsTabHelper(), future.GetCallback());
    ASSERT_TRUE(NavigateTo("www.youtube.com", "/@testuser"));
    EXPECT_EQ(future.Get(), "youtube#channel:987654321");
  }

  {
    // Same-document navigation via `history.pushState`.
    TestFuture<std::string> future;
    TabHelperObserver observer(GetRewardsTabHelper(), future.GetCallback());
    auto* web_contents = chrome_test_utils::GetActiveWebContents(this);
    EXPECT_TRUE(content::ExecJs(web_contents, "triggerSameDocNav()"));
    EXPECT_EQ(future.Get(), "youtube#channel:123456789");
  }

  {
    // Navigation away from the page.
    TestFuture<std::string> future;
    TabHelperObserver observer(GetRewardsTabHelper(), future.GetCallback());
    ASSERT_TRUE(NavigateTo("abc.youtube.com", "/@testuser"));
    WaitForTimeout();
    EXPECT_EQ(GetRewardsTabHelper().GetPublisherIdForTab(), "");
  }

  // Ensure that publisher info is stored in the Rewards database appropriately.
  auto info = WaitForPublisherInfo("youtube#channel:987654321");
  ASSERT_TRUE(info);
  EXPECT_EQ(info->name, "testuser");
  EXPECT_EQ(info->url, embedded_https_test_server().GetURL("www.youtube.com",
                                                           "/@testuser"));
}

IN_PROC_BROWSER_TEST_F(CreatorDetectionBrowserTest, InvalidHost) {
  EnableRewards();
  SetResponseHTML(kYouTubeHTML);
  AddBlankTabAndShow(browser());
  ASSERT_TRUE(NavigateTo("abc.youtube.com", "/@testuser"));
  WaitForTimeout();
  EXPECT_EQ(GetRewardsTabHelper().GetPublisherIdForTab(), "");
}

IN_PROC_BROWSER_TEST_F(CreatorDetectionBrowserTest, RewardsDisabled) {
  SetResponseHTML(kYouTubeHTML);
  AddBlankTabAndShow(browser());
  ASSERT_TRUE(NavigateTo("www.youtube.com", "/@testuser"));
  WaitForTimeout();
  EXPECT_EQ(GetRewardsTabHelper().GetPublisherIdForTab(), "");
}

IN_PROC_BROWSER_TEST_F(CreatorDetectionBrowserTest, IncognitoProfile) {
  EnableRewards();
  SetResponseHTML(kYouTubeHTML);
  auto* incognito_browser = CreateIncognitoBrowser();
  AddBlankTabAndShow(incognito_browser);
  auto url =
      embedded_https_test_server().GetURL("www.youtube.com", "/@testuser");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(incognito_browser, url));
  WaitForTimeout();
  EXPECT_EQ(GetRewardsTabHelper().GetPublisherIdForTab(), "");
}

class CreatorDetectionFeatureDisabledBrowserTest
    : public CreatorDetectionBrowserTest {
 public:
  CreatorDetectionFeatureDisabledBrowserTest() { DisableFeature(); }
};

IN_PROC_BROWSER_TEST_F(CreatorDetectionFeatureDisabledBrowserTest,
                       FeatureDisabled) {
  EnableRewards();
  SetResponseHTML(kYouTubeHTML);
  AddBlankTabAndShow(browser());
  ASSERT_TRUE(NavigateTo("www.youtube.com", "/@testuser"));
  WaitForTimeout();
  EXPECT_EQ(GetRewardsTabHelper().GetPublisherIdForTab(), "");
}

}  // namespace brave_rewards
