// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <string>

#include "base/functional/bind.h"
#include "base/run_loop.h"
#include "base/test/ios/wait_util.h"
#include "brave/ios/web/test/grit/test_resources.h"
#include "brave/ios/web/webui/brave_web_ui_ios_data_source.h"
#include "ios/web/public/navigation/navigation_manager.h"
#include "ios/web/public/test/navigation_test_util.h"
#include "ios/web/public/test/web_test_with_web_state.h"
#include "ios/web/public/test/web_view_content_test_util.h"
#include "ios/web/public/test/web_view_interaction_test_util.h"
#include "ios/web/public/webui/web_ui_ios_controller.h"
#include "ios/web/public/webui/web_ui_ios_controller_factory.h"
#include "ios/web/public/webui/web_ui_ios_data_source.h"
#include "ios/web/test/test_url_constants.h"
#include "ios/web/web_state/web_state_impl.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "url/gurl.h"
#include "url/scheme_host_port.h"

using base::test::ios::kWaitForPageLoadTimeout;
using base::test::ios::WaitUntilConditionOrTimeout;
using web::test::WaitForWebViewContainingText;
using web::test::WaitForWebViewContainingTextInFrame;

namespace web {

namespace {

// Hostname for test WebUI page.
const char kTestWebUIURLHost[] = "testwebui";
const char kTestWebUIURLHost2[] = "testwebui2";

// Text present on the sample WebUI page.
const char kWebUIPageText[] = "WebUI page";
const char kWebUIPage2Text[] = "WebUI 2 page";

// Controller for test WebUI.
class TestUI : public WebUIIOSController {
 public:
  // Constructs controller from `web_ui` and `ui_handler` which will communicate
  // with test WebUI page.
  TestUI(WebUIIOS* web_ui, const std::string& host, int resource_id)
      : WebUIIOSController(web_ui, host) {
    // Need to use a BraveWebUIIOSDataSource instead of standard
    // WebUIIOSDataSource because we the test html has an iframe pointing to
    // WebUI and so we need to override the CSP
    BraveWebUIIOSDataSource* source = BraveWebUIIOSDataSource::Create(host);

    source->SetDefaultResource(resource_id);
    source->OverrideContentSecurityPolicy(
        network::mojom::CSPDirectiveName::FrameSrc,
        absl::StrFormat("frame-src %s://%s", kTestWebUIScheme,
                        kTestWebUIURLHost2));

    web::WebState* web_state = web_ui->GetWebState();
    web::WebUIIOSDataSource::Add(web_state->GetBrowserState(), source);
  }

  ~TestUI() override = default;
};

// Factory that creates TestUI controller.
class TestWebUIControllerFactory : public WebUIIOSControllerFactory {
 public:
  // Constructs a controller factory.
  TestWebUIControllerFactory() {}

  // WebUIIOSControllerFactory overrides.
  std::unique_ptr<WebUIIOSController> CreateWebUIIOSControllerForURL(
      WebUIIOS* web_ui,
      const GURL& url) const override {
    if (!url.SchemeIs(kTestWebUIScheme)) {
      return nullptr;
    }
    if (url.host() == kTestWebUIURLHost) {
      return std::make_unique<TestUI>(web_ui, std::string(url.host()),
                                      IDR_WEBUI_TEST_HTML);
    }
    DCHECK_EQ(url.host(), kTestWebUIURLHost2);
    return std::make_unique<TestUI>(web_ui, std::string(url.host()),
                                    IDR_WEBUI_TEST_HTML_2);
  }

  NSInteger GetErrorCodeForWebUIURL(const GURL& url) const override {
    if (url.SchemeIs(kTestWebUIScheme)) {
      return 0;
    }
    return NSURLErrorUnsupportedURL;
  }
};
}  // namespace

// A test fixture for verifying WebUI. This test fixture is copied from
// //ios/web/webui/web_ui_inttest.mm
class WebUITest : public WebTestWithWebState {
 protected:
  WebUITest() : WebTestWithWebState() {}

  void SetUp() override {
    WebTestWithWebState::SetUp();
    factory_ = std::make_unique<TestWebUIControllerFactory>();
    WebUIIOSControllerFactory::RegisterFactory(factory_.get());

    url::SchemeHostPort tuple(kTestWebUIScheme, kTestWebUIURLHost, 0);
    GURL url(tuple.Serialize());
    test::LoadUrl(web_state(), url);

    // LoadIfNecessary is needed because the view is not created (but needed)
    // when loading the page. TODO(crbug.com/41309809): Remove this call.
    web_state()->GetNavigationManager()->LoadIfNecessary();

    ASSERT_TRUE(WaitUntilConditionOrTimeout(kWaitForPageLoadTimeout, ^{
      base::RunLoop().RunUntilIdle();
      return !web_state()->IsLoading();
    }));

    ASSERT_EQ(url.spec(), BaseUrl());
  }

  void TearDown() override {
    WebUIIOSControllerFactory::DeregisterFactory(factory_.get());
    WebTestWithWebState::TearDown();
  }

 private:
  std::unique_ptr<TestWebUIControllerFactory> factory_;
};

// Tests that a both the main web UI page and its child frame also containing
// WebUI both load correctly and that WebState is holding onto both of the
// WebUIIOS references
TEST_F(WebUITest, LoadWebUIPageWithWebUIChildFrame) {
  auto* web_state_impl = WebStateImpl::FromWebState(web_state());
  ASSERT_TRUE(web_state_impl->HasWebUI());
  EXPECT_TRUE(WaitForWebViewContainingText(web_state(), kWebUIPageText));
  ASSERT_TRUE(web_state_impl->GetMainFrameWebUI() != nullptr);
  EXPECT_TRUE(
      WaitForWebViewContainingTextInFrame(web_state(), kWebUIPage2Text));
  size_t expected_web_ui_count = 2;
  ASSERT_EQ(web_state_impl->GetWebUICountForTesting(), expected_web_ui_count);
}

}  // namespace web
