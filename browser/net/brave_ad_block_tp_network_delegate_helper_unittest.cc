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
#include "brave/common/network_constants.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/ad_block_subscription_download_manager.h"
#include "brave/components/brave_shields/browser/ad_block_subscription_service_manager.h"
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

namespace {

// Note: extract a common impl if needed for other tests, do not copy.

// TODO(iefremov): This is only needed to provide a task runner to the adblock
// service. We can drop this stub once the service doesn't need an
// "external" runner.
class TestingBraveComponentUpdaterDelegate : public BraveComponent::Delegate {
 public:
  explicit TestingBraveComponentUpdaterDelegate(PrefService* local_state)
      : local_state_(local_state) {}
  ~TestingBraveComponentUpdaterDelegate() override = default;

  TestingBraveComponentUpdaterDelegate(TestingBraveComponentUpdaterDelegate&) =
      delete;
  TestingBraveComponentUpdaterDelegate& operator=(
      TestingBraveComponentUpdaterDelegate&) = delete;

  using ComponentObserver = update_client::UpdateClient::Observer;

  // brave_component_updater::BraveComponent::Delegate implementation
  void Register(const std::string& component_name,
                const std::string& component_base64_public_key,
                base::OnceClosure registered_callback,
                BraveComponent::ReadyCallback ready_callback) override {}
  bool Unregister(const std::string& component_id) override { return true; }
  void OnDemandUpdate(const std::string& component_id) override {}

  void AddObserver(ComponentObserver* observer) override {}
  void RemoveObserver(ComponentObserver* observer) override {}

  scoped_refptr<base::SequencedTaskRunner> GetTaskRunner() override {
    return base::ThreadTaskRunnerHandle::Get();
  }

  const std::string locale() const override { return "en"; }
  PrefService* local_state() override { return local_state_; }

 private:
  raw_ptr<PrefService> local_state_ = nullptr;
};

}  // namespace

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

    brave_component_updater_delegate_ =
        std::make_unique<TestingBraveComponentUpdaterDelegate>(
            local_state_->Get());

    base::FilePath user_data_dir;
    DCHECK(base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir));
    auto adblock_service = std::make_unique<brave_shields::AdBlockService>(
        brave_component_updater_delegate_.get(),
        std::make_unique<brave_shields::AdBlockSubscriptionServiceManager>(
            brave_component_updater_delegate_.get(),
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

  void ResetAdblockInstance(brave_shields::AdBlockBaseService* service,
                            std::string rules,
                            std::string resources,
                            bool include_redirect_urls) {
    service->ResetForTest(rules, resources, include_redirect_urls);
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

  std::unique_ptr<TestingBraveComponentUpdaterDelegate>
      brave_component_updater_delegate_;

  content::BrowserTaskEnvironment task_environment_;

  std::unique_ptr<net::MockHostResolver> host_resolver_;

  std::unique_ptr<StubResolverConfigReader> stub_resolver_config_reader_;

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
  ResetAdblockInstance(g_brave_browser_process->ad_block_service(),
                       "||brave.com/test.txt", "", false);

  const GURL url("https://brave.com/test.txt");
  auto request_info = std::make_shared<brave::BraveRequestInfo>(url);
  request_info->request_identifier = 1;
  request_info->resource_type = blink::mojom::ResourceType::kScript;
  request_info->initiator_url = GURL("https://brave.com");

  EXPECT_TRUE(CheckRequest(request_info));
  EXPECT_EQ(request_info->blocked_by, brave::kAdBlocked);
  EXPECT_TRUE(request_info->new_url_spec.empty());
  // It's unclear whether or not this is a Tor request, so no DNS queries are
  // made (`browser_context` is `nullptr`).
  EXPECT_EQ(0ULL, host_resolver_->num_resolve());
}

TEST_F(BraveAdBlockTPNetworkDelegateHelperTest, RedirectUrl) {
  ResetAdblockInstance(
      g_brave_browser_process->ad_block_service(),
      "||brave.com/test.js$redirect-url=https://pcdn.brave.com/test.js", "",
      true);

  const GURL url("https://brave.com/test.js");
  auto request_info = std::make_shared<brave::BraveRequestInfo>(url);
  request_info->request_identifier = 1;
  request_info->resource_type = blink::mojom::ResourceType::kScript;
  request_info->initiator_url = GURL("https://brave.com");

  EXPECT_TRUE(CheckRequest(request_info));
  EXPECT_EQ(request_info->blocked_by, brave::kAdBlocked);
  EXPECT_EQ("https://pcdn.brave.com/test.js", request_info->new_url_spec);
}

TEST_F(BraveAdBlockTPNetworkDelegateHelperTest, NotPrivateCDNDomain) {
  ResetAdblockInstance(
      g_brave_browser_process->ad_block_service(),
      "||brave.com/test.js$redirect-url=https://test.brave.com/test.js", "",
      true);

  const GURL url("https://brave.com/test.js");
  auto request_info = std::make_shared<brave::BraveRequestInfo>(url);
  request_info->request_identifier = 1;
  request_info->resource_type = blink::mojom::ResourceType::kScript;
  request_info->initiator_url = GURL("https://brave.com");

  EXPECT_TRUE(CheckRequest(request_info));
  EXPECT_EQ(request_info->blocked_by, brave::kNotBlocked);
  EXPECT_EQ("", request_info->new_url_spec);
}

TEST_F(BraveAdBlockTPNetworkDelegateHelperTest, RedirectUrlCommaTwoUrls) {
  ResetAdblockInstance(g_brave_browser_process->ad_block_service(),
                       "||brave.com/test.js$redirect-url=https://test.com/"
                       "test.js,https://test.com/test2.js",
                       "", true);

  const GURL url("https://brave.com/test.js");
  auto request_info = std::make_shared<brave::BraveRequestInfo>(url);
  request_info->request_identifier = 1;
  request_info->resource_type = blink::mojom::ResourceType::kScript;
  request_info->initiator_url = GURL("https://brave.com");

  EXPECT_TRUE(CheckRequest(request_info));
  EXPECT_EQ(request_info->blocked_by, brave::kNotBlocked);
  EXPECT_EQ("", request_info->new_url_spec);
}

TEST_F(BraveAdBlockTPNetworkDelegateHelperTest, RedirectUrlCommaInFilename) {
  ResetAdblockInstance(
      g_brave_browser_process->ad_block_service(),
      "||brave.com/test.js$redirect-url=https://test.com/tes,t.js", "", true);

  const GURL url("https://brave.com/test.js");
  auto request_info = std::make_shared<brave::BraveRequestInfo>(url);
  request_info->request_identifier = 1;
  request_info->resource_type = blink::mojom::ResourceType::kScript;
  request_info->initiator_url = GURL("https://brave.com");

  EXPECT_TRUE(CheckRequest(request_info));
  EXPECT_EQ(request_info->blocked_by, brave::kNotBlocked);
  EXPECT_EQ("", request_info->new_url_spec);
}

TEST_F(BraveAdBlockTPNetworkDelegateHelperTest, RedirectUrlCommaInPath) {
  ResetAdblockInstance(
      g_brave_browser_process->ad_block_service(),
      "||brave.com/test.js$redirect-url=https://test.com/xy,z/test.js", "",
      true);

  const GURL url("https://brave.com/test.js");
  auto request_info = std::make_shared<brave::BraveRequestInfo>(url);
  request_info->request_identifier = 1;
  request_info->resource_type = blink::mojom::ResourceType::kScript;
  request_info->initiator_url = GURL("https://brave.com");

  EXPECT_TRUE(CheckRequest(request_info));
  EXPECT_EQ(request_info->blocked_by, brave::kNotBlocked);
  EXPECT_EQ("", request_info->new_url_spec);
}

TEST_F(BraveAdBlockTPNetworkDelegateHelperTest, RedirectUrlCommaInRule) {
  ResetAdblockInstance(g_brave_browser_process->ad_block_service(),
                       "||brave.com/test.js$redirect-url=https://"
                       "pcdn.bravesoftware.com/xyz/test.js,script",
                       "", true);

  const GURL url("https://brave.com/test.js");
  auto request_info = std::make_shared<brave::BraveRequestInfo>(url);
  request_info->request_identifier = 1;
  request_info->resource_type = blink::mojom::ResourceType::kScript;
  request_info->initiator_url = GURL("https://brave.com");

  EXPECT_TRUE(CheckRequest(request_info));
  EXPECT_EQ(request_info->blocked_by, brave::kAdBlocked);
  EXPECT_EQ("https://pcdn.bravesoftware.com/xyz/test.js",
            request_info->new_url_spec);
}

TEST_F(BraveAdBlockTPNetworkDelegateHelperTest, RedirectUrlNoHttps) {
  ResetAdblockInstance(
      g_brave_browser_process->ad_block_service(),
      "||brave.com/test.js$redirect-url=http://test.com/test.js", "", true);

  const GURL url("https://brave.com/test.js");
  auto request_info = std::make_shared<brave::BraveRequestInfo>(url);
  request_info->request_identifier = 1;
  request_info->resource_type = blink::mojom::ResourceType::kScript;
  request_info->initiator_url = GURL("https://brave.com");

  EXPECT_TRUE(CheckRequest(request_info));
  EXPECT_TRUE(request_info->new_url_spec.empty());
  EXPECT_EQ(request_info->blocked_by, brave::kNotBlocked);
}
