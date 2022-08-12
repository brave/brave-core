/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_ad_block_tp_network_delegate_helper.h"

#include <memory>
#include <string>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/threading/thread_task_runner_handle.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/net/url_context.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/ad_block_subscription_download_manager.h"
#include "brave/components/brave_shields/browser/ad_block_subscription_service_manager.h"
#include "brave/components/brave_shields/browser/test_filters_provider.h"
#include "brave/components/constants/network_constants.h"
#include "brave/test/base/testing_brave_browser_process.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "content/public/test/browser_task_environment.h"
#include "net/base/net_errors.h"
#include "net/dns/mock_host_resolver.h"
#include "net/log/net_log.h"
#include "services/network/host_resolver.h"
#include "testing/gtest/include/gtest/gtest.h"

using brave::ResponseCallback;
using brave_shields::TestFiltersProvider;

void FakeAdBlockSubscriptionDownloadManagerGetter(
    base::OnceCallback<
        void(brave_shields::AdBlockSubscriptionDownloadManager*)>) {
  // no-op, subscription services are not currently used in unit tests
}

class BraveAdBlockTPNetworkDelegateHelperTest : public testing::Test {
 protected:
  void SetUp() override {
    local_state_ = std::make_unique<ScopedTestingLocalState>(
        TestingBrowserProcess::GetGlobal());

    base::FilePath user_data_dir;
    DCHECK(base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir));
    auto adblock_service = std::make_unique<brave_shields::AdBlockService>(
        local_state_->Get(), "en", nullptr, base::ThreadTaskRunnerHandle::Get(),
        std::make_unique<brave_shields::AdBlockSubscriptionServiceManager>(
            local_state_->Get(), base::ThreadTaskRunnerHandle::Get(),
            base::BindOnce(&FakeAdBlockSubscriptionDownloadManagerGetter),
            user_data_dir));

    TestingBraveBrowserProcess::GetGlobal()->SetAdBlockService(
        std::move(adblock_service));

    g_brave_browser_process->ad_block_service()->Start();

    host_resolver_ = std::make_unique<net::MockHostResolver>();
    resolver_wrapper_ = std::make_unique<network::HostResolver>(
        host_resolver_.get(), net::NetLog::Get());
    brave::SetAdblockCnameHostResolverForTesting(resolver_wrapper_.get());

    stub_resolver_config_reader_ =
        std::make_unique<StubResolverConfigReader>(local_state_->Get());
    SystemNetworkContextManager::set_stub_resolver_config_reader_for_testing(
        stub_resolver_config_reader_.get());
  }

  void TearDown() override {
    // The AdBlockBaseService destructor must be called before the task runner
    // is destroyed.
    TestingBraveBrowserProcess::DeleteInstance();
  }

  void ResetAdblockInstance(std::string rules, std::string resources) {
    filters_provider_ = std::make_unique<TestFiltersProvider>(rules, resources);
    g_brave_browser_process->ad_block_service()->UseSourceProvidersForTest(
        filters_provider_.get(), filters_provider_.get());
  }

  // Returns true if the request handler deferred control back to the calling
  // thread before completion, or true if it completed instantly.
  bool CheckRequest(std::shared_ptr<brave::BraveRequestInfo> request_info) {
    // `request_identifier` must be nonzero, or else nothing will be tested.
    request_info->request_identifier = 1;

    int rc =
        OnBeforeURLRequest_AdBlockTPPreWork(base::DoNothing(), request_info);
    EXPECT_TRUE(rc == net::OK || rc == net::ERR_IO_PENDING);
    task_environment_.RunUntilIdle();

    return rc == net::ERR_IO_PENDING;
  }

  std::unique_ptr<ScopedTestingLocalState> local_state_;

  content::BrowserTaskEnvironment task_environment_;

  std::unique_ptr<net::MockHostResolver> host_resolver_;

  std::unique_ptr<StubResolverConfigReader> stub_resolver_config_reader_;

  std::unique_ptr<TestFiltersProvider> filters_provider_;

 private:
  std::unique_ptr<network::HostResolver> resolver_wrapper_;
};

TEST_F(BraveAdBlockTPNetworkDelegateHelperTest, NoInitiatorURL) {
  const GURL url("https://bradhatesprimes.brave.com/composite_numbers_ftw");
  auto request_info = std::make_shared<brave::BraveRequestInfo>(url);
  request_info->resource_type = blink::mojom::ResourceType::kScript;

  EXPECT_FALSE(CheckRequest(request_info));
  EXPECT_EQ(request_info->blocked_by, brave::kNotBlocked);
  EXPECT_TRUE(request_info->new_url_spec.empty());
}

TEST_F(BraveAdBlockTPNetworkDelegateHelperTest, EmptyRequestURL) {
  auto request_info = std::make_shared<brave::BraveRequestInfo>(GURL());
  request_info->initiator_url = GURL("https://example.com");
  request_info->resource_type = blink::mojom::ResourceType::kScript;

  EXPECT_FALSE(CheckRequest(request_info));
  EXPECT_EQ(request_info->blocked_by, brave::kNotBlocked);
  EXPECT_TRUE(request_info->new_url_spec.empty());
}

TEST_F(BraveAdBlockTPNetworkDelegateHelperTest, DevToolURL) {
  const GURL url("devtools://devtools/");
  auto request_info = std::make_shared<brave::BraveRequestInfo>(url);
  request_info->initiator_url =
      GURL("devtools://devtools/bundled/root/root.js");
  request_info->resource_type = blink::mojom::ResourceType::kScript;

  EXPECT_FALSE(CheckRequest(request_info));
  EXPECT_EQ(request_info->blocked_by, brave::kNotBlocked);
  EXPECT_TRUE(request_info->new_url_spec.empty());
}

TEST_F(BraveAdBlockTPNetworkDelegateHelperTest, RequestDataURL) {
  const GURL url(
      "data:image/gif;base64,R0lGODlhAQABAIAAAP///"
      "wAAACH5BAEAAAAALAAAAAABAAEAAAICRAEAOw==");
  auto request_info = std::make_shared<brave::BraveRequestInfo>(url);
  request_info->initiator_url = GURL("https://example.com");
  request_info->resource_type = blink::mojom::ResourceType::kImage;

  EXPECT_FALSE(CheckRequest(request_info));
  EXPECT_EQ(request_info->blocked_by, brave::kNotBlocked);
  EXPECT_TRUE(request_info->new_url_spec.empty());
}

TEST_F(BraveAdBlockTPNetworkDelegateHelperTest, SimpleBlocking) {
  ResetAdblockInstance("||brave.com/test.txt", "");

  const GURL url("https://brave.com/test.txt");
  auto request_info = std::make_shared<brave::BraveRequestInfo>(url);
  request_info->request_identifier = 1;
  request_info->resource_type = blink::mojom::ResourceType::kScript;
  request_info->initiator_url = GURL("https://bravesoftware.com");

  EXPECT_TRUE(CheckRequest(request_info));
  EXPECT_EQ(request_info->blocked_by, brave::kAdBlocked);
  EXPECT_TRUE(request_info->new_url_spec.empty());
  // It's unclear whether or not this is a Tor request, so no DNS queries are
  // made (`browser_context` is `nullptr`).
  EXPECT_EQ(0ULL, host_resolver_->num_resolve());
}

TEST_F(BraveAdBlockTPNetworkDelegateHelperTest, Default1pException) {
  ResetAdblockInstance("||brave.com/test.txt", "");

  const GURL url("https://brave.com/test.txt");
  auto request_info = std::make_shared<brave::BraveRequestInfo>(url);
  request_info->request_identifier = 1;
  request_info->resource_type = blink::mojom::ResourceType::kScript;
  request_info->initiator_url = GURL("https://brave.com");

  EXPECT_TRUE(CheckRequest(request_info));
  EXPECT_EQ(request_info->blocked_by, brave::kNotBlocked);
  EXPECT_TRUE(request_info->new_url_spec.empty());
  EXPECT_EQ(0ULL, host_resolver_->num_resolve());
}

TEST_F(BraveAdBlockTPNetworkDelegateHelperTest, AggressiveNo1pException) {
  ResetAdblockInstance("||brave.com/test.txt", "");

  const GURL url("https://brave.com/test.txt");
  auto request_info = std::make_shared<brave::BraveRequestInfo>(url);
  request_info->request_identifier = 1;
  request_info->resource_type = blink::mojom::ResourceType::kScript;
  request_info->initiator_url = GURL("https://brave.com");
  request_info->aggressive_blocking = true;

  EXPECT_TRUE(CheckRequest(request_info));
  EXPECT_EQ(request_info->blocked_by, brave::kAdBlocked);
  EXPECT_TRUE(request_info->new_url_spec.empty());
  // It's unclear whether or not this is a Tor request, so no DNS queries are
  // made (`browser_context` is `nullptr`).
  EXPECT_EQ(0ULL, host_resolver_->num_resolve());
}
