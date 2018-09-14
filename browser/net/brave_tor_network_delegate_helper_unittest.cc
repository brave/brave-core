/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_tor_network_delegate_helper.h"

#include "brave/browser/net/url_context.h"
#include "brave/browser/renderer_host/brave_navigation_ui_data.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/test/mock_resource_context.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_test_util.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace {

int kRenderProcessId = 1;
int kRenderFrameId = 2;

class BraveTorNetworkDelegateHelperTest: public testing::Test {
 public:
  BraveTorNetworkDelegateHelperTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP),
        context_(new net::TestURLRequestContext(true)) {
  }
  ~BraveTorNetworkDelegateHelperTest() override {}
  void SetUp() override {
    context_->Init();
  }
  net::TestURLRequestContext* context() { return context_.get(); }

  content::MockResourceContext* resource_context() {
    return resource_context_.get();
  }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<net::TestURLRequestContext> context_;
  std::unique_ptr<content::MockResourceContext> resource_context_;
};

TEST_F(BraveTorNetworkDelegateHelperTest, NotTorProfile) {
  net::TestDelegate test_delegate;
  GURL url("https://check.torproject.org/");
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(url, net::IDLE, &test_delegate,
                             TRAFFIC_ANNOTATION_FOR_TESTS);
  std::shared_ptr<brave::BraveRequestInfo>
      before_url_context(new brave::BraveRequestInfo());
  brave::ResponseCallback callback;

  std::unique_ptr<BraveNavigationUIData> navigation_ui_data =
    std::make_unique<BraveNavigationUIData>();
  content::ResourceRequestInfo::AllocateForTesting(
    request.get(), content::RESOURCE_TYPE_MAIN_FRAME, resource_context(),
    kRenderProcessId, /*render_view_id=*/-1, kRenderFrameId,
    /*is_main_frame=*/true, /*allow_download=*/false, /*is_async=*/true,
    content::PREVIEWS_OFF, std::move(navigation_ui_data));
  GURL new_url;
  int ret =
    brave::OnBeforeURLRequest_TorWork(request.get(), &new_url, callback,
                                      before_url_context);
  EXPECT_TRUE(new_url.is_empty());
  auto* proxy_service = request->context()->proxy_resolution_service();
  EXPECT_FALSE(proxy_service->config());
  EXPECT_EQ(ret, net::OK);
}

// TODO: TEST_F(BraveTorNetworkDelegateHelperTest, TorProfile) {}
// have to conquer kProfileUsingTor registration in TestingProfile
// and prevent TorLauncherFactory launching with TorProfileServiceImpl

}  // namespace
