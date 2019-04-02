/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_tor_network_delegate_helper.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "brave/browser/net/url_context.h"
#include "brave/browser/profiles/brave_profile_manager.h"
#include "brave/browser/profiles/tor_unittest_profile_manager.h"
#include "brave/browser/renderer_host/brave_navigation_ui_data.h"
#include "brave/browser/tor/mock_tor_profile_service_factory.h"
#include "brave/common/tor/tor_common.h"
#include "brave/common/tor/tor_test_constants.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/test/mock_resource_context.h"
#include "content/public/test/test_utils.h"
#include "net/log/net_log_with_source.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_test_util.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace {

int kRenderProcessId = 1;
int kRenderFrameId = 2;

}  // namespace

class BraveTorNetworkDelegateHelperTest: public testing::Test {
 public:
  BraveTorNetworkDelegateHelperTest()
      : local_state_(TestingBrowserProcess::GetGlobal()),
        thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP),
        context_(new net::TestURLRequestContext(true)) {}

  ~BraveTorNetworkDelegateHelperTest() override {}

  void SetUp() override {
    // Create a new temporary directory, and store the path
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    TestingBrowserProcess::GetGlobal()->SetProfileManager(
        new TorUnittestProfileManager(temp_dir_.GetPath()));
    context_->Init();
  }

  void TearDown() override {
    TestingBrowserProcess::GetGlobal()->SetProfileManager(nullptr);
    content::RunAllTasksUntilIdle();
  }

  net::TestURLRequestContext* context() { return context_.get(); }

  content::MockResourceContext* resource_context() {
    return resource_context_.get();
  }

 protected:
  // The path to temporary directory used to contain the test operations.
  base::ScopedTempDir temp_dir_;
  ScopedTestingLocalState local_state_;

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<net::TestURLRequestContext> context_;
  std::unique_ptr<content::MockResourceContext> resource_context_;
  DISALLOW_COPY_AND_ASSIGN(BraveTorNetworkDelegateHelperTest);
};

TEST_F(BraveTorNetworkDelegateHelperTest, NotTorProfile) {
  net::TestDelegate test_delegate;
  GURL url("https://check.torproject.org/");
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(url, net::IDLE, &test_delegate,
                             TRAFFIC_ANNOTATION_FOR_TESTS);
  std::shared_ptr<brave::BraveRequestInfo>
      before_url_context(new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(request.get(),
                                              before_url_context);
  brave::ResponseCallback callback;

  std::unique_ptr<BraveNavigationUIData> navigation_ui_data =
    std::make_unique<BraveNavigationUIData>();
  content::ResourceRequestInfo::AllocateForTesting(
      request.get(), content::RESOURCE_TYPE_MAIN_FRAME, resource_context(),
      kRenderProcessId, /*render_view_id=*/-1, kRenderFrameId,
      /*is_main_frame=*/true, content::ResourceInterceptPolicy::kAllowNone,
      /*is_async=*/true, content::PREVIEWS_OFF, std::move(navigation_ui_data));
  int ret =
    brave::OnBeforeURLRequest_TorWork(callback,
                                      before_url_context);
  EXPECT_TRUE(before_url_context->new_url_spec.empty());
  auto* proxy_service = request->context()->proxy_resolution_service();
  net::ProxyInfo info;
  std::unique_ptr<net::ProxyResolutionService::Request> proxy_request;
  proxy_service->ResolveProxy(url, std::string(), &info, base::DoNothing(),
                              &proxy_request, net::NetLogWithSource());
  EXPECT_EQ(info.ToPacString(), "DIRECT");
  ASSERT_TRUE(proxy_service->config());
  EXPECT_TRUE(proxy_service->config()->value().proxy_rules().empty());
  EXPECT_EQ(ret, net::OK);
}

TEST_F(BraveTorNetworkDelegateHelperTest, TorProfile) {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  base::FilePath tor_path = BraveProfileManager::GetTorProfilePath();

  Profile* profile = profile_manager->GetProfile(tor_path);
  ASSERT_TRUE(profile);

  net::TestDelegate test_delegate;
  GURL url("https://check.torproject.org/");
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(url, net::IDLE, &test_delegate,
                             TRAFFIC_ANNOTATION_FOR_TESTS);
  std::shared_ptr<brave::BraveRequestInfo>
      before_url_context(new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(request.get(),
                                              before_url_context);
  brave::ResponseCallback callback;

  std::unique_ptr<BraveNavigationUIData> navigation_ui_data =
    std::make_unique<BraveNavigationUIData>();
  BraveNavigationUIData* navigation_ui_data_ptr = navigation_ui_data.get();
  content::ResourceRequestInfo::AllocateForTesting(
      request.get(), content::RESOURCE_TYPE_MAIN_FRAME, resource_context(),
      kRenderProcessId, /*render_view_id=*/-1, kRenderFrameId,
      /*is_main_frame=*/true, content::ResourceInterceptPolicy::kAllowNone,
      /*is_async=*/true, content::PREVIEWS_OFF, std::move(navigation_ui_data));

  MockTorProfileServiceFactory::SetTorNavigationUIData(profile,
                                                   navigation_ui_data_ptr);
  int ret =
    brave::OnBeforeURLRequest_TorWork(callback,
                                      before_url_context);
  EXPECT_TRUE(before_url_context->new_url_spec.empty());
  auto* proxy_service = request->context()->proxy_resolution_service();
  net::ProxyInfo info;
  std::unique_ptr<net::ProxyResolutionService::Request> proxy_request;
  proxy_service->ResolveProxy(url, std::string(), &info, base::DoNothing(),
                              &proxy_request, net::NetLogWithSource());
  EXPECT_EQ(info.ToPacString(), tor::kTestTorPacString);
  ASSERT_TRUE(proxy_service->config());
  ASSERT_FALSE(proxy_service->config()->value().proxy_rules().empty());
  net::ProxyConfig::ProxyRules rules;
  rules.ParseFromString(tor::kTestTorProxy);
  EXPECT_TRUE(proxy_service->config()->value().proxy_rules().Equals(rules));
  EXPECT_EQ(ret, net::OK);
}

TEST_F(BraveTorNetworkDelegateHelperTest, TorProfileBlockFile) {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  base::FilePath tor_path = BraveProfileManager::GetTorProfilePath();

  Profile* profile = profile_manager->GetProfile(tor_path);
  ASSERT_TRUE(profile);

  net::TestDelegate test_delegate;
  GURL url("file://test");
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(url, net::IDLE, &test_delegate,
                             TRAFFIC_ANNOTATION_FOR_TESTS);
  std::shared_ptr<brave::BraveRequestInfo>
      before_url_context(new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(request.get(),
                                              before_url_context);
  brave::ResponseCallback callback;

  std::unique_ptr<BraveNavigationUIData> navigation_ui_data =
    std::make_unique<BraveNavigationUIData>();
  BraveNavigationUIData* navigation_ui_data_ptr = navigation_ui_data.get();
  content::ResourceRequestInfo::AllocateForTesting(
      request.get(), content::RESOURCE_TYPE_MAIN_FRAME, resource_context(),
      kRenderProcessId, /*render_view_id=*/-1, kRenderFrameId,
      /*is_main_frame=*/true, content::ResourceInterceptPolicy::kAllowNone,
      /*is_async=*/true, content::PREVIEWS_OFF, std::move(navigation_ui_data));

  MockTorProfileServiceFactory::SetTorNavigationUIData(profile,
                                                   navigation_ui_data_ptr);
  int ret =
    brave::OnBeforeURLRequest_TorWork(callback,
                                      before_url_context);
  EXPECT_TRUE(before_url_context->new_url_spec.empty());
  EXPECT_EQ(ret, net::ERR_DISALLOWED_URL_SCHEME);
}

TEST_F(BraveTorNetworkDelegateHelperTest, TorProfileBlockIfHosed) {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  base::FilePath tor_path = BraveProfileManager::GetTorProfilePath();

  Profile* profile = profile_manager->GetProfile(tor_path);
  ASSERT_TRUE(profile);

  net::TestDelegate test_delegate;
  GURL url("https://check.torproject.org/");
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(url, net::IDLE, &test_delegate,
                             TRAFFIC_ANNOTATION_FOR_TESTS);
  std::shared_ptr<brave::BraveRequestInfo>
      before_url_context(new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(request.get(),
                                              before_url_context);
  brave::ResponseCallback callback;

  std::unique_ptr<BraveNavigationUIData> navigation_ui_data =
    std::make_unique<BraveNavigationUIData>();
  BraveNavigationUIData* navigation_ui_data_ptr = navigation_ui_data.get();
  content::ResourceRequestInfo::AllocateForTesting(
      request.get(), content::RESOURCE_TYPE_MAIN_FRAME, resource_context(),
      kRenderProcessId, /*render_view_id=*/-1, kRenderFrameId,
      /*is_main_frame=*/true, content::ResourceInterceptPolicy::kAllowNone,
      /*is_async=*/true, content::PREVIEWS_OFF, std::move(navigation_ui_data));

  MockTorProfileServiceFactory::SetTorNavigationUIData(profile,
                                                       navigation_ui_data_ptr);

  // `Relaunch' tor with broken config.
  {
    auto* tor_profile_service = navigation_ui_data_ptr->GetTorProfileService();
    base::FilePath path(tor::kTestBrokenTorPath);
    std::string proxy(tor::kTestTorProxy);
    tor_profile_service->ReLaunchTor(tor::TorConfig(path, proxy));
  }

  int ret =
    brave::OnBeforeURLRequest_TorWork(callback,
                                      before_url_context);
  EXPECT_TRUE(before_url_context->new_url_spec.empty());
  EXPECT_NE(ret, net::OK);
}
