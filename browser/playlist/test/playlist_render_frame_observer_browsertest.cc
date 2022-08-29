/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/components/playlist/features.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

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
    scoped_feature_list_.InitWithFeatures(
        {playlist::features::kPlaylist,
         playlist::features::kCompareOnlyHostForTesting},
        {});
  }
  ~PlaylistRenderFrameObserverBrowserTest() override = default;

  void CheckMediaSourceAPI(const GURL& url, APIVisibility visibility) {
    const auto* test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    VLOG(2) << test_info->name() << ": " << __func__;

    ASSERT_TRUE(url.is_valid());

    auto* active_web_contents = chrome_test_utils::GetActiveWebContents(this);
    // This is blocking call.
    ASSERT_TRUE(content::NavigateToURL(active_web_contents, url));
    EXPECT_EQ(visibility == APIVisibility::kVisible,
              EvalJs(active_web_contents, "!!window.MediaSource"));
  }

 protected:
  // PlatformBrowserTest:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    PlatformBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }
  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");

    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_->RegisterRequestHandler(
        base::BindRepeating(&PlaylistRenderFrameObserverBrowserTest::Serve,
                            base::Unretained(this)));
    ASSERT_TRUE(https_server_->Start());
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

  void TearDownOnMainThread() override {
    ASSERT_TRUE(https_server_->ShutdownAndWaitUntilComplete());

    PlatformBrowserTest::TearDownOnMainThread();
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

IN_PROC_BROWSER_TEST_F(PlaylistRenderFrameObserverBrowserTest,
                       CheckNormalSites) {
  CheckMediaSourceAPI(https_server_->GetURL("www.a.com", "/"),
                      APIVisibility::kVisible);
}

#if BUILDFLAG(IS_ANDROID)
// TODO(sko): Fix this test on Android.
// https://github.com/brave/brave-browser/issues/24971
#define CheckYoutube DISABLED_CheckYoutube
#else
#define CheckYoutube CheckYoutube
#endif
IN_PROC_BROWSER_TEST_F(PlaylistRenderFrameObserverBrowserTest, CheckYoutube) {
  CheckMediaSourceAPI(https_server_->GetURL("www.youtube.com", "/"),
                      APIVisibility::kHidden);
}
