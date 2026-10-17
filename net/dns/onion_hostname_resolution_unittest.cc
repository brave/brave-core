/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <optional>
#include <string_view>

#include "base/memory/scoped_refptr.h"
#include "base/test/task_environment.h"
#include "net/base/host_port_pair.h"
#include "net/base/net_errors.h"
#include "net/base/network_anonymization_key.h"
#include "net/base/network_handle.h"
#include "net/base/test_completion_callback.h"
#include "net/dns/mock_host_resolver.h"
#include "net/log/net_log_with_source.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/scheme_host_port.h"

namespace net {

class OnionHostnameResolutionTest : public testing::Test {
 public:
  void SetUp() override {
    host_resolver_proc_ =
        base::MakeRefCounted<RuleBasedHostResolverProc>(nullptr);
    host_resolver_proc_->AddRule("*", "127.0.0.1");
    scoped_host_resolver_proc_.Init(host_resolver_proc_.get());

    auto builder = CreateTestURLRequestContextBuilder();
    builder->set_host_resolver(HostResolver::CreateStandaloneContextResolver(
        /*net_log=*/nullptr, HostResolver::ManagerOptions()));
    context_ = builder->Build();
  }

  int Resolve(std::string_view host,
              std::optional<HostResolver::ResolveHostParameters> parameters =
                  std::nullopt) {
    std::unique_ptr<HostResolver::ResolveHostRequest> request =
        context_->host_resolver()->CreateRequest(
            HostPortPair(host, 80), NetworkAnonymizationKey(),
            handles::kInvalidNetworkHandle, NetLogWithSource(), parameters);
    TestCompletionCallback callback;
    return callback.GetResult(request->Start(callback.callback()));
  }

  int ResolveURL(std::string_view url) {
    std::unique_ptr<HostResolver::ResolveHostRequest> request =
        context_->host_resolver()->CreateRequest(
            url::SchemeHostPort(GURL(url)), NetworkAnonymizationKey(),
            handles::kInvalidNetworkHandle, NetLogWithSource(), std::nullopt);
    TestCompletionCallback callback;
    return callback.GetResult(request->Start(callback.callback()));
  }

  size_t NumSystemResolves() {
    return host_resolver_proc_->NumResolvesForHostPattern("*");
  }

 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::MainThreadType::IO};
  scoped_refptr<RuleBasedHostResolverProc> host_resolver_proc_;
  ScopedDefaultHostResolverProc scoped_host_resolver_proc_;
  std::unique_ptr<URLRequestContext> context_;
};

TEST_F(OnionHostnameResolutionTest, BlocksByDefault) {
  EXPECT_FALSE(context_->is_tor());
  EXPECT_TRUE(context_->onion_only_in_tor_windows());

  EXPECT_EQ(ERR_NAME_NOT_RESOLVED, Resolve("example.onion"));
  EXPECT_EQ(0u, NumSystemResolves());
}

TEST_F(OnionHostnameResolutionTest, ResolvesOnionWhenAllowedOutsideTor) {
  context_->set_onion_only_in_tor_windows(false);

  EXPECT_EQ(OK, Resolve("example.onion"));
  EXPECT_EQ(1u, NumSystemResolves());
}

TEST_F(OnionHostnameResolutionTest, ResolvesOnionInTorContext) {
  context_->set_is_tor(true);

  EXPECT_EQ(OK, Resolve("example.onion"));
  EXPECT_EQ(1u, NumSystemResolves());
}

TEST_F(OnionHostnameResolutionTest, FailsOnionWhenBlocked) {
  EXPECT_EQ(ERR_NAME_NOT_RESOLVED, Resolve("example.onion"));
  EXPECT_EQ(ERR_NAME_NOT_RESOLVED, Resolve("sub.example.onion"));
  EXPECT_EQ(ERR_NAME_NOT_RESOLVED, Resolve("EXAMPLE.ONION"));
  EXPECT_EQ(ERR_NAME_NOT_RESOLVED, Resolve("example.onion."));
  EXPECT_EQ(ERR_NAME_NOT_RESOLVED, Resolve("onion"));
  EXPECT_EQ(ERR_NAME_NOT_RESOLVED, ResolveURL("https://example.onion/"));
  EXPECT_EQ(ERR_NAME_NOT_RESOLVED, ResolveURL("wss://example.onion/"));
  EXPECT_EQ(0u, NumSystemResolves());
}

TEST_F(OnionHostnameResolutionTest, ResolvesOtherHostsWhenBlocked) {
  EXPECT_EQ(OK, Resolve("example.com"));
  EXPECT_EQ(OK, Resolve("example.onion.com"));
  EXPECT_EQ(OK, Resolve("exampleonion"));
  EXPECT_EQ(OK, ResolveURL("https://example.com/"));
  EXPECT_EQ(4u, NumSystemResolves());
}

TEST_F(OnionHostnameResolutionTest, ServesCachedOnionResultsWhenBlocked) {
  context_->set_onion_only_in_tor_windows(false);
  EXPECT_EQ(OK, Resolve("example.onion"));
  EXPECT_EQ(1u, NumSystemResolves());

  context_->set_onion_only_in_tor_windows(true);
  EXPECT_EQ(OK, Resolve("example.onion"));

  HostResolver::ResolveHostParameters parameters;
  parameters.cache_usage =
      HostResolver::ResolveHostParameters::CacheUsage::DISALLOWED;
  EXPECT_EQ(ERR_NAME_NOT_RESOLVED, Resolve("example.onion", parameters));
  EXPECT_EQ(1u, NumSystemResolves());
}

}  // namespace net
