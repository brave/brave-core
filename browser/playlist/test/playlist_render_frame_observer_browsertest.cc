/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/components/playlist/browser/playlist_service.h"
#include "brave/components/playlist/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "services/network/public/cpp/network_switches.h"

#if BUILDFLAG(IS_ANDROID)
#include "chrome/test/base/android/android_browser_test.h"
#else
#include "chrome/test/base/in_process_browser_test.h"
#endif

class PlaylistRenderFrameObserverBrowserTest : public PlatformBrowserTest {
 public:
  enum class APIVisibility {
    kVisible,
    kHidden,
  };

  PlaylistRenderFrameObserverBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(playlist::features::kPlaylist);

    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_->RegisterRequestHandler(
        base::BindRepeating(&PlaylistRenderFrameObserverBrowserTest::Serve,
                            base::Unretained(this)));
    embedded_test_server()->RegisterRequestHandler(
        base::BindRepeating(&PlaylistRenderFrameObserverBrowserTest::Serve,
                            base::Unretained(this)));
    EXPECT_TRUE(https_server_->Start());
    EXPECT_TRUE(embedded_test_server()->Start());
  }

  ~PlaylistRenderFrameObserverBrowserTest() override = default;

  void CheckMediaSourceAPI(const GURL& url, APIVisibility visibility) {
    const auto* test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    VLOG(2) << test_info->name() << ": " << __func__;

    ASSERT_TRUE(url.is_valid());

    // We shouldn't hide Media src API from tabs' web contents. It has many
    // downsides.
    auto* active_web_contents = chrome_test_utils::GetActiveWebContents(this);
    ASSERT_TRUE(content::NavigateToURL(active_web_contents, url));
    EXPECT_TRUE(
        EvalJs(active_web_contents, "!!window.MediaSource").ExtractBool());

    auto* playlist_service =
        playlist::PlaylistServiceFactory::GetForBrowserContext(
            chrome_test_utils::GetProfile(this));

    // Instead, we should download contents on backgrounds when we should hide
    // the API.
    EXPECT_EQ(visibility == APIVisibility::kHidden,
              playlist_service->ShouldGetMediaFromBackgroundWebContents(
                  active_web_contents));

    // Then, the WebContents used for background download always hides the API.
    auto* background_web_contents = playlist_service->download_request_manager_
                                        ->GetBackgroundWebContentsForTesting();
    ASSERT_TRUE(content::NavigateToURL(background_web_contents, url));
    EXPECT_FALSE(
        EvalJs(background_web_contents, "!!window.MediaSource").ExtractBool());
  }

 protected:
  // PlatformBrowserTest:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    ASSERT_TRUE(https_server_);
    PlatformBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);

    command_line->AppendSwitchASCII(
        network::switches::kHostResolverRules,
        base::StringPrintf("MAP *:80 127.0.0.1:%d,"
                           "MAP *:443 127.0.0.1:%d",
                           embedded_test_server()->port(),
                           https_server_->port()));
  }
  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
  }

  void SetUpInProcessBrowserTestFixture() override {
    PlatformBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    PlatformBrowserTest::TearDownInProcessBrowserTestFixture();
  }

 protected:
  std::unique_ptr<net::EmbeddedTestServer> https_server_;

 private:
  std::unique_ptr<net::test_server::HttpResponse> Serve(
      const net::test_server::HttpRequest& request) {
    auto response = std::make_unique<net::test_server::BasicHttpResponse>();
    response->set_code(net::HTTP_OK);
    response->set_content_type("text/html; charset=utf-8");
    return response;
  }

  base::test::ScopedFeatureList scoped_feature_list_;

  content::ContentMockCertVerifier mock_cert_verifier_;
};

#if BUILDFLAG(IS_ANDROID)
// TODO(sko): Fix this test on Android.
// https://github.com/brave/brave-browser/issues/24971
#define CheckNormalSites DISABLED_CheckNormalSites
#else
#define CheckNormalSites CheckNormalSites
#endif
IN_PROC_BROWSER_TEST_F(PlaylistRenderFrameObserverBrowserTest,
                       CheckNormalSites) {
  CheckMediaSourceAPI(GURL("http://a.com/"), APIVisibility::kVisible);
}

#if BUILDFLAG(IS_ANDROID)
// TODO(sko): Fix this test on Android.
// https://github.com/brave/brave-browser/issues/24971
#define CheckYoutube DISABLED_CheckYoutube
#else
#define CheckYoutube CheckYoutube
#endif
IN_PROC_BROWSER_TEST_F(PlaylistRenderFrameObserverBrowserTest, CheckYoutube) {
  CheckMediaSourceAPI(GURL("https://www.youtube.com/"), APIVisibility::kHidden);
  CheckMediaSourceAPI(GURL("https://youtube.com/"), APIVisibility::kHidden);
}
