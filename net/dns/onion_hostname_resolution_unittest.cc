/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "net/base/address_family.h"
#include "net/base/host_port_pair.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/base/network_anonymization_key.h"
#include "net/base/network_handle.h"
#include "net/base/test_completion_callback.h"
#include "net/dns/context_host_resolver.h"
#include "net/dns/dns_config.h"
#include "net/dns/dns_hosts.h"
#include "net/dns/dns_test_util.h"
#include "net/dns/host_resolver.h"
#include "net/dns/host_resolver_manager.h"
#include "net/dns/host_resolver_system_task.h"
#include "net/dns/mock_host_resolver.h"
#include "net/dns/public/dns_over_https_config.h"
#include "net/dns/public/dns_protocol.h"
#include "net/dns/public/secure_dns_mode.h"
#include "net/dns/public/secure_dns_policy.h"
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

  int Resolve(std::string_view host) {
    std::unique_ptr<HostResolver::ResolveHostRequest> request =
        context_->host_resolver()->CreateRequest(
            HostPortPair(host, 80), NetworkAnonymizationKey(),
            handles::kInvalidNetworkHandle, NetLogWithSource(), std::nullopt);
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

#if BUILDFLAG(ENABLE_TOR)

TEST_F(OnionHostnameResolutionTest, BlocksByDefault) {
  EXPECT_EQ(URLRequestContext::TorMode::kDisabled, context_->tor_mode());

  EXPECT_EQ(ERR_NAME_NOT_RESOLVED, Resolve("example.onion"));
  EXPECT_EQ(0u, NumSystemResolves());
}

TEST_F(OnionHostnameResolutionTest, ResolvesOnionInTorContext) {
  context_->set_tor_mode(URLRequestContext::TorMode::kEnabled);

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

// Resolves through a MockDnsClient, so lookups can take the built-in DNS and
// DoH paths. The system resolver is `host_resolver_proc_`.
class OnionHostnameResolutionDnsClientTest : public testing::Test {
 public:
  void SetUp() override {
    host_resolver_proc_ =
        base::MakeRefCounted<RuleBasedHostResolverProc>(nullptr);
    host_resolver_proc_->AddRule("*", "127.0.0.1");

    std::unique_ptr<ContextHostResolver> host_resolver =
        HostResolver::CreateStandaloneContextResolver(
            /*net_log=*/nullptr, HostResolver::ManagerOptions());
    host_resolver->SetHostResolverSystemParamsForTest(
        HostResolverSystemTask::Params(host_resolver_proc_,
                                       /*max_retry_attempts=*/0));
    host_resolver_ = host_resolver.get();

    auto builder = CreateTestURLRequestContextBuilder();
    builder->set_host_resolver(std::move(host_resolver));
    context_ = builder->Build();
  }

  void TearDown() override {
    dns_client_ = nullptr;
    host_resolver_ = nullptr;
  }

  // Uses `secure_dns_mode` with a DoH server, `hosts` as the hosts file and
  // `rules` for DNS and DoH transactions. Insecure built-in DNS is enabled when
  // `insecure_dns` is set; otherwise insecure lookups use the system resolver.
  void SetUpDnsClient(SecureDnsMode secure_dns_mode,
                      MockDnsClientRuleList rules,
                      bool insecure_dns = false,
                      DnsHosts hosts = {}) {
    DnsConfig config({IPEndPoint(IPAddress(192, 168, 1, 1), 53)});
    config.doh_config =
        *DnsOverHttpsConfig::FromString("https://doh.test/dns-query");
    config.secure_dns_mode = secure_dns_mode;
    config.hosts = std::move(hosts);

    auto dns_client =
        std::make_unique<MockDnsClient>(std::move(config), std::move(rules));
    dns_client->set_ignore_system_config_changes(true);
    if (insecure_dns) {
      dns_client->SetInsecureEnabled(InsecureDnsMode::kEnabledBuiltIn,
                                     /*additional_types_enabled=*/false);
    }
    dns_client_ = dns_client.get();
    host_resolver_->GetManagerForTesting()->SetDnsClientForTesting(
        std::move(dns_client));
  }

  std::unique_ptr<HostResolver::ResolveHostRequest> CreateRequest(
      url::SchemeHostPort host,
      SecureDnsPolicy secure_dns_policy = SecureDnsPolicy::kAllow) {
    HostResolver::ResolveHostParameters parameters;
    parameters.dns_query_type = DnsQueryType::A;
    parameters.secure_dns_policy = secure_dns_policy;
    return context_->host_resolver()->CreateRequest(
        std::move(host), NetworkAnonymizationKey(),
        handles::kInvalidNetworkHandle, NetLogWithSource(), parameters);
  }

  std::unique_ptr<HostResolver::ResolveHostRequest> CreateRequest(
      std::string_view host) {
    return CreateRequest(url::SchemeHostPort(url::kHttpsScheme, host, 443));
  }

  size_t NumSystemResolves() {
    return host_resolver_proc_->NumResolvesForHostPattern("*");
  }

  // Sets up `secure_dns_mode` and, if `insecure_dns`, built-in insecure DNS.
  // DNS and DoH transactions answer example.com and fail the test for
  // example.onion.
  void SetUpExampleDnsClient(SecureDnsMode secure_dns_mode, bool insecure_dns) {
    MockDnsClientRuleList rules;
    for (bool secure : {false, true}) {
      rules.emplace_back(
          "example.com", dns_protocol::kTypeA, secure,
          MockDnsClientRule::Result(MockDnsClientRule::ResultType::kOk),
          /*delay=*/false);
      rules.emplace_back(
          "example.onion", dns_protocol::kTypeA, secure,
          MockDnsClientRule::Result(MockDnsClientRule::ResultType::kUnexpected),
          /*delay=*/false);
    }
    SetUpDnsClient(secure_dns_mode, std::move(rules), insecure_dns);
  }

  int ResolveHost(std::string_view host) {
    std::unique_ptr<HostResolver::ResolveHostRequest> request =
        CreateRequest(host);
    TestCompletionCallback callback;
    return callback.GetResult(request->Start(callback.callback()));
  }

  int ResolveServiceEndpoints(std::string_view host) {
    class Delegate : public HostResolver::ServiceEndpointRequest::Delegate {
     public:
      void OnServiceEndpointsUpdated() override {}
      void OnServiceEndpointRequestFinished(int rv) override {
        result.SetValue(rv);
      }
      base::test::TestFuture<int> result;
    };

    HostResolver::ResolveHostParameters parameters;
    parameters.dns_query_type = DnsQueryType::A;
    std::unique_ptr<HostResolver::ServiceEndpointRequest> request =
        context_->host_resolver()->CreateServiceEndpointRequest(
            HostResolver::Host(
                url::SchemeHostPort(url::kHttpsScheme, host, 443)),
            NetworkAnonymizationKey(), handles::kInvalidNetworkHandle,
            NetLogWithSource(), parameters);
    Delegate delegate;
    const int rv = request->Start(&delegate);
    return rv == ERR_IO_PENDING ? delegate.result.Get() : rv;
  }

  // example.com must be answered by a DNS or DoH transaction, which shows the
  // mode uses them; example.onion must fail without starting one or falling
  // back to the system resolver.
  void ExpectOnionBlockedBeforeDns(SecureDnsMode secure_dns_mode,
                                   bool insecure_dns) {
    SetUpExampleDnsClient(secure_dns_mode, insecure_dns);

    EXPECT_EQ(OK, ResolveHost("example.com"));
    EXPECT_EQ(ERR_NAME_NOT_RESOLVED, ResolveHost("example.onion"));
    EXPECT_EQ(0u, NumSystemResolves());
  }

 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::MainThreadType::IO};
  scoped_refptr<RuleBasedHostResolverProc> host_resolver_proc_;
  std::unique_ptr<URLRequestContext> context_;
  raw_ptr<ContextHostResolver> host_resolver_ = nullptr;
  raw_ptr<MockDnsClient> dns_client_ = nullptr;
};

TEST_F(OnionHostnameResolutionDnsClientTest, BlocksInsecureBuiltInDns) {
  ExpectOnionBlockedBeforeDns(SecureDnsMode::kOff, /*insecure_dns=*/true);
}

TEST_F(OnionHostnameResolutionDnsClientTest, BlocksAutomaticDoh) {
  ExpectOnionBlockedBeforeDns(SecureDnsMode::kAutomatic, /*insecure_dns=*/true);
}

TEST_F(OnionHostnameResolutionDnsClientTest, BlocksSecureDoh) {
  ExpectOnionBlockedBeforeDns(SecureDnsMode::kSecure, /*insecure_dns=*/false);
}

// The HTTP stream pool resolves through service endpoint requests.
TEST_F(OnionHostnameResolutionDnsClientTest, BlocksServiceEndpointRequest) {
  SetUpExampleDnsClient(SecureDnsMode::kAutomatic, /*insecure_dns=*/true);

  EXPECT_EQ(OK, ResolveServiceEndpoints("example.com"));
  EXPECT_EQ(ERR_NAME_NOT_RESOLVED, ResolveServiceEndpoints("example.onion"));
  EXPECT_EQ(0u, NumSystemResolves());
}

// The DoH follow-up of a bootstrap lookup answered from preset addresses must
// not send a blocked .onion name.
TEST_F(OnionHostnameResolutionDnsClientTest, BlocksBootstrapFollowup) {
  MockDnsClientRuleList rules;
  rules.emplace_back(
      "doh.example.onion", dns_protocol::kTypeA,
      /*secure=*/true,
      MockDnsClientRule::Result(MockDnsClientRule::ResultType::kOk),
      /*delay=*/true);
  SetUpDnsClient(SecureDnsMode::kAutomatic, std::move(rules));
  const url::SchemeHostPort endpoint(url::kHttpsScheme, "doh.example.onion",
                                     443);
  dns_client_->set_preset_endpoint(endpoint);
  dns_client_->set_preset_addrs({IPEndPoint(IPAddress(192, 0, 2, 1), 443)});

  std::unique_ptr<HostResolver::ResolveHostRequest> request =
      CreateRequest(endpoint, SecureDnsPolicy::kBootstrap);
  TestCompletionCallback callback;
  EXPECT_EQ(OK, callback.GetResult(request->Start(callback.callback())));

  EXPECT_FALSE(
      dns_client_->CompleteOneDelayedTransactionOfType(DnsQueryType::A));
}

// When the system resolver would run next, the hosts file is skipped in
// favor of getaddrinfo(). A blocked lookup still gets the hosts file answer.
TEST_F(OnionHostnameResolutionDnsClientTest, ServesHostsFileWhenBlocked) {
  DnsHosts hosts;
  hosts[DnsHostsKey("example.onion", ADDRESS_FAMILY_IPV4)] =
      IPAddress(10, 0, 0, 1);
  SetUpDnsClient(SecureDnsMode::kOff, MockDnsClientRuleList(),
                 /*insecure_dns=*/false, std::move(hosts));

  std::unique_ptr<HostResolver::ResolveHostRequest> request =
      CreateRequest("example.onion");
  TestCompletionCallback callback;
  EXPECT_EQ(OK, callback.GetResult(request->Start(callback.callback())));
  ASSERT_EQ(1u, request->GetAddressResults().size());
  EXPECT_EQ(IPAddress(10, 0, 0, 1),
            request->GetAddressResults().front().address());
  EXPECT_EQ(0u, NumSystemResolves());
}

#else  // BUILDFLAG(ENABLE_TOR)

// Builds without Tor never block .onion, even in a context left at the default
// kDisabled.
TEST_F(OnionHostnameResolutionTest, ResolvesOnionWithoutTor) {
  EXPECT_EQ(URLRequestContext::TorMode::kDisabled, context_->tor_mode());

  EXPECT_EQ(OK, Resolve("example.onion"));
  EXPECT_EQ(1u, NumSystemResolves());
}

#endif  // BUILDFLAG(ENABLE_TOR)

}  // namespace net
