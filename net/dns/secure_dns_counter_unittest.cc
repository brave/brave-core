/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/net/dns/secure_dns_counter.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "net/base/net_errors.h"
#include "net/base/network_anonymization_key.h"
#include "net/base/network_handle.h"
#include "net/base/test_completion_callback.h"
#include "net/dns/dns_response.h"
#include "net/dns/dns_test_util.h"
#include "net/dns/host_resolver.h"
#include "net/dns/host_resolver_manager.h"
#include "net/dns/host_resolver_manager_unittest.h"
#include "net/dns/public/dns_config_overrides.h"
#include "net/dns/public/dns_protocol.h"
#include "net/dns/public/dns_query_type.h"
#include "net/dns/public/secure_dns_mode.h"
#include "net/log/net_log_with_source.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/scheme_host_port.h"
#include "url/url_constants.h"

namespace net {

class SecureDnsCounterHostResolverTest : public HostResolverManagerDnsTest {
 protected:
  static void AddHttpsRule(MockDnsClientRuleList* rules,
                           const std::string& host,
                           bool secure) {
    std::vector<DnsResourceRecord> records = {
        BuildTestHttpsServiceRecord(host, /*priority=*/1, /*service_name=*/".",
                                    /*params=*/{})};
    rules->emplace_back(host, dns_protocol::kTypeHttps, secure,
                        MockDnsClientRule::Result(BuildTestDnsResponse(
                            host, dns_protocol::kTypeHttps, records)),
                        /*delay=*/false);
  }

  int ResolveHttps(const std::string& host) {
    HostResolver::ResolveHostParameters parameters;
    parameters.dns_query_type = DnsQueryType::HTTPS;
    std::unique_ptr<HostResolver::ResolveHostRequest> request =
        resolver_->CreateRequest(
            url::SchemeHostPort(url::kHttpsScheme, host, 443),
            NetworkAnonymizationKey(), handles::kInvalidNetworkHandle,
            NetLogWithSource(), parameters, resolve_context_.get());
    TestCompletionCallback callback;
    return callback.GetResult(request->Start(callback.callback()));
  }
};

TEST_F(SecureDnsCounterHostResolverTest, CountsHttpsJobsByTaskType) {
  SecureDnsCounter::GetInstance()->GetCountsAndReset();

  MockDnsClientRuleList rules;
  AddHttpsRule(&rules, "secure.test", /*secure=*/true);
  AddHttpsRule(&rules, "insecure.test", /*secure=*/false);
  CreateResolver();
  UseMockDnsClient(CreateValidDnsConfig(), std::move(rules));
  DnsConfigOverrides overrides;
  overrides.secure_dns_mode = SecureDnsMode::kAutomatic;
  resolver_->SetDnsConfigOverrides(overrides);

  ASSERT_EQ(OK, ResolveHttps("secure.test"));
  DnsRequestCounts counts =
      SecureDnsCounter::GetInstance()->GetCountsAndReset();
  EXPECT_EQ(1u, counts.total_count);
  EXPECT_EQ(1u, counts.upgraded_count);

  ASSERT_EQ(OK, ResolveHttps("insecure.test"));
  counts = SecureDnsCounter::GetInstance()->GetCountsAndReset();
  EXPECT_EQ(1u, counts.total_count);
  EXPECT_EQ(0u, counts.upgraded_count);
}

}  // namespace net
