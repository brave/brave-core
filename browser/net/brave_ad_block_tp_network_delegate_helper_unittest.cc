/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_ad_block_tp_network_delegate_helper.h"

#include <memory>
#include <string>
#include <vector>

#include "brave/browser/net/url_context.h"
#include "brave/common/network_constants.h"
#include "chrome/browser/net/secure_dns_config.h"
#include "chrome/browser/net/stub_resolver_config_reader.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "net/base/net_errors.h"
#include "testing/gtest/include/gtest/gtest.h"

using brave::ResponseCallback;

TEST(BraveAdBlockTPNetworkDelegateHelperTest, NoChangeURL) {
  const GURL url("https://bradhatesprimes.brave.com/composite_numbers_ftw");
  auto request_info = std::make_shared<brave::BraveRequestInfo>(url);
  int rc =
      OnBeforeURLRequest_AdBlockTPPreWork(ResponseCallback(), request_info);
  EXPECT_TRUE(request_info->new_url_spec.empty());
  EXPECT_EQ(rc, net::OK);
}

TEST(BraveAdBlockTPNetworkDelegateHelperTest, EmptyRequestURL) {
  auto request_info = std::make_shared<brave::BraveRequestInfo>(GURL());
  int rc =
      OnBeforeURLRequest_AdBlockTPPreWork(ResponseCallback(), request_info);
  EXPECT_TRUE(request_info->new_url_spec.empty());
  EXPECT_EQ(rc, net::OK);
}

namespace {
class TestTorBrowserContext : public content::TestBrowserContext {
 public:
  TestTorBrowserContext() : content::TestBrowserContext() {}
  ~TestTorBrowserContext() override = default;

  bool IsTor() const override { return true; }
};
}  // namespace

class BraveCNAMEAdBlockTest : public testing::Test {
 public:
  BraveCNAMEAdBlockTest() : tor_browser_context_(new TestTorBrowserContext()) {
    local_state_ = std::make_unique<ScopedTestingLocalState>(
        TestingBrowserProcess::GetGlobal());

    // SystemNetworkContextManager cannot be instantiated here, which normally
    // owns the StubResolverConfigReader instance, so inject a
    // StubResolverConfigReader instance here.
    stub_resolver_config_reader_ =
        std::make_unique<StubResolverConfigReader>(local_state_->Get());
    SystemNetworkContextManager::set_stub_resolver_config_reader_for_testing(
        stub_resolver_config_reader_.get());
  }
  ~BraveCNAMEAdBlockTest() override = default;

  content::BrowserContext* tor_context() { return tor_browser_context_.get(); }

  TestingPrefServiceSimple* local_state() { return local_state_->Get(); }

 private:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<content::BrowserContext> tor_browser_context_;
  std::unique_ptr<ScopedTestingLocalState> local_state_;
  std::unique_ptr<StubResolverConfigReader> stub_resolver_config_reader_;
};

TEST_F(BraveCNAMEAdBlockTest, DisableInsecureRequestsOverTor) {
  const GURL url("https://wow.brave.com");
  auto request_info = std::make_shared<brave::BraveRequestInfo>(url);
  request_info->tab_origin = GURL("https://brave.com");
  request_info->resource_type = blink::mojom::ResourceType::kMainFrame;
  request_info->browser_context = tor_context();

  // Automatic
  local_state()->SetManagedPref(
      prefs::kDnsOverHttpsMode,
      std::make_unique<base::Value>(SecureDnsConfig::kModeAutomatic));
  int rc =
      OnBeforeURLRequest_AdBlockTPPreWork(ResponseCallback(), request_info);
  EXPECT_EQ(rc, net::OK);

  // Off
  local_state()->SetManagedPref(
      prefs::kDnsOverHttpsMode,
      std::make_unique<base::Value>(SecureDnsConfig::kModeOff));
  rc = OnBeforeURLRequest_AdBlockTPPreWork(ResponseCallback(), request_info);
  EXPECT_EQ(rc, net::OK);

  // Secure without server list
  local_state()->SetManagedPref(
      prefs::kDnsOverHttpsMode,
      std::make_unique<base::Value>(SecureDnsConfig::kModeSecure));
  rc = OnBeforeURLRequest_AdBlockTPPreWork(ResponseCallback(), request_info);
  EXPECT_EQ(rc, net::OK);
}
