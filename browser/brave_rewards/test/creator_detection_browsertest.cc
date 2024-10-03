// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <string>
#include <string_view>

#include "base/memory/raw_ref.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/test/test_timeouts.h"
#include "base/timer/timer.h"
#include "brave/browser/brave_rewards/rewards_tab_helper.h"
#include "brave/components/brave_rewards/common/features.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"

using net::test_server::BasicHttpResponse;
using net::test_server::HttpRequest;
using net::test_server::HttpResponse;

namespace brave_rewards {

class CreatorDetectionBrowserTest : public InProcessBrowserTest {
 protected:
  CreatorDetectionBrowserTest() {
    scoped_feature_list_.InitWithFeatures(
        {features::kPlatformCreatorDetectionFeature}, {});
  }

  ~CreatorDetectionBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");
    embedded_https_test_server().SetCertHostnames(
        {"twitter.com", "github.com", "api.github.com", "reddit.com",
         "www.twitch.tv", "vimeo.com", "www.youtube.com"});
    embedded_https_test_server().RegisterRequestHandler(base::BindRepeating(
        &CreatorDetectionBrowserTest::HandleRequest, base::Unretained(this)));
    ASSERT_TRUE(embedded_https_test_server().Start());
  }

  void TearDownOnMainThread() override {
    EXPECT_TRUE(embedded_https_test_server().ShutdownAndWaitUntilComplete());
    InProcessBrowserTest::TearDownOnMainThread();
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

  void SetRewardsEnabled() {
    auto* prefs = browser()->profile()->GetPrefs();
    prefs->SetBoolean(brave_rewards::prefs::kEnabled, true);
  }

  void NavigateTo(std::string_view host, std::string_view path) {
    auto url = embedded_https_test_server().GetURL(host, path);
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  }

  RewardsTabHelper& GetRewardsTabHelper() {
    auto* web_contents = chrome_test_utils::GetActiveWebContents(this);
    CHECK(web_contents);
    auto* tab_helper = RewardsTabHelper::FromWebContents(web_contents);
    CHECK(tab_helper);
    return *tab_helper;
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

IN_PROC_BROWSER_TEST_F(CreatorDetectionBrowserTest, GithubDetection) {
  SetRewardsEnabled();

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
  base::test::TestFuture<std::string> future;
  TabHelperObserver observer(GetRewardsTabHelper(), future.GetCallback());
  NavigateTo("github.com", "/testuser");

  EXPECT_EQ(future.Get<0>(), "github#channel:1234567");
}

IN_PROC_BROWSER_TEST_F(CreatorDetectionBrowserTest, RedditDetection) {
  SetRewardsEnabled();

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
  base::test::TestFuture<std::string> future;
  TabHelperObserver observer(GetRewardsTabHelper(), future.GetCallback());
  NavigateTo("reddit.com", "/user/testuser");

  EXPECT_EQ(future.Get<0>(), "reddit#channel:987654321");
}

IN_PROC_BROWSER_TEST_F(CreatorDetectionBrowserTest, TwitchDetection) {
  SetRewardsEnabled();

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
  base::test::TestFuture<std::string> future;
  TabHelperObserver observer(GetRewardsTabHelper(), future.GetCallback());
  NavigateTo("www.twitch.tv", "/testuser");

  EXPECT_EQ(future.Get<0>(), "twitch#author:testuser");
}

IN_PROC_BROWSER_TEST_F(CreatorDetectionBrowserTest, TwitterDetection) {
  SetRewardsEnabled();

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
  base::test::TestFuture<std::string> future;
  TabHelperObserver observer(GetRewardsTabHelper(), future.GetCallback());
  NavigateTo("twitter.com", "/testuser");

  EXPECT_EQ(future.Get<0>(), "twitter#channel:987654321");
}

IN_PROC_BROWSER_TEST_F(CreatorDetectionBrowserTest, VimeoDetection) {
  SetRewardsEnabled();

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
  base::test::TestFuture<std::string> future;
  TabHelperObserver observer(GetRewardsTabHelper(), future.GetCallback());
  NavigateTo("vimeo.com", "/testuser");

  EXPECT_EQ(future.Get<0>(), "vimeo#channel:987654321");
}

constexpr std::string_view kYouTubeHTML = R"(
  <!doctype html>
  <html>
  <head>
    <link rel="canonical" href="/channel/987654321" />
  </head>
  <body>
    <ytd-video-owner-renderer>
      <a href="/@testuser"></a>
    </ytd-video-owner-renderer>
    <div id="avatar">
      <img src="/user-avatar" />
    </div>
  </body>
  </html>
)";

IN_PROC_BROWSER_TEST_F(CreatorDetectionBrowserTest, YouTubeDetection) {
  SetRewardsEnabled();
  SetResponseHTML(kYouTubeHTML);

  AddBlankTabAndShow(browser());
  base::test::TestFuture<std::string> future;
  TabHelperObserver observer(GetRewardsTabHelper(), future.GetCallback());
  NavigateTo("www.youtube.com", "/@testuser");

  EXPECT_EQ(future.Get<0>(), "youtube#channel:987654321");
}

IN_PROC_BROWSER_TEST_F(CreatorDetectionBrowserTest, InvalidHost) {
  SetRewardsEnabled();
  SetResponseHTML(kYouTubeHTML);
  AddBlankTabAndShow(browser());

  base::RunLoop run_loop;
  base::OneShotTimer timeout;
  timeout.Start(FROM_HERE, TestTimeouts::action_timeout(),
                run_loop.QuitClosure());

  NavigateTo("abc.youtube.com", "/@testuser");
  EXPECT_EQ(GetRewardsTabHelper().GetPublisherIdForTab(), "");
}

IN_PROC_BROWSER_TEST_F(CreatorDetectionBrowserTest, RewardsDisabled) {
  SetResponseHTML(kYouTubeHTML);
  AddBlankTabAndShow(browser());

  base::RunLoop run_loop;
  base::OneShotTimer timeout;
  timeout.Start(FROM_HERE, TestTimeouts::action_timeout(),
                run_loop.QuitClosure());

  NavigateTo("www.youtube.com", "/@testuser");
  EXPECT_EQ(GetRewardsTabHelper().GetPublisherIdForTab(), "");
}

}  // namespace brave_rewards
