/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/net/dns/brave_resolve_context.h"

#include <memory>

#include "base/bind.h"
#include "base/test/task_environment.h"
#include "brave/net/decentralized_dns/constants.h"
#include "net/base/net_errors.h"
#include "net/dns/dns_config.h"
#include "net/dns/dns_server_iterator.h"
#include "net/dns/dns_session.h"
#include "net/dns/dns_socket_allocator.h"
#include "net/dns/public/dns_over_https_server_config.h"
#include "net/url_request/url_request_context.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

namespace {

DnsConfig CreateDnsConfig() {
  DnsConfig config;

  config.dns_over_https_servers.push_back(*DnsOverHttpsServerConfig::FromString(
      decentralized_dns::kUnstoppableDomainsDoHResolver));
  config.dns_over_https_servers.push_back(*DnsOverHttpsServerConfig::FromString(
      decentralized_dns::kENSDoHResolver));

  return config;
}

class BraveResolveContextTest : public testing::Test {
 public:
  BraveResolveContextTest() = default;

  scoped_refptr<DnsSession> CreateDnsSession(const DnsConfig& config) {
    // Session not expected to be used for anything that will actually
    // require random numbers.
    auto null_random_callback =
        base::BindRepeating([](int, int) -> int { IMMEDIATE_CRASH(); });

    return base::MakeRefCounted<DnsSession>(
        config, nullptr /* socket_allocator */, null_random_callback,
        nullptr /* netlog */);
  }

 private:
  base::test::TaskEnvironment task_environment_;
};

TEST_F(BraveResolveContextTest, DohServerAvailability_InitialAvailability) {
  DnsConfig config = CreateDnsConfig();
  scoped_refptr<DnsSession> session = CreateDnsSession(config);

  URLRequestContext request_context;
  BraveResolveContext context(&request_context, true /* enable_caching */);
  context.InvalidateCachesAndPerSessionData(session.get(),
                                            false /* network_change */);

  EXPECT_EQ(context.NumAvailableDohServers(session.get()), 2u);
  EXPECT_TRUE(context.GetDohServerAvailability(0u, session.get()));
  EXPECT_TRUE(context.GetDohServerAvailability(1u, session.get()));

  std::unique_ptr<DnsServerIterator> doh_itr = context.GetDohIterator(
      session->config(), SecureDnsMode::kAutomatic, session.get());
  EXPECT_TRUE(doh_itr->AttemptAvailable());
}

TEST_F(BraveResolveContextTest, DohServerAvailability_RecordServerFailure) {
  scoped_refptr<DnsSession> session = CreateDnsSession(CreateDnsConfig());

  URLRequestContext request_context;
  BraveResolveContext context(&request_context, true /* enable_caching */);
  context.InvalidateCachesAndPerSessionData(session.get(),
                                            false /* network_change */);

  context.RecordServerFailure(0u /* server_index */, true /* is_doh_server */,
                              ERR_FAILED, session.get());
  EXPECT_EQ(context.NumAvailableDohServers(session.get()), 1u);
  EXPECT_FALSE(context.GetDohServerAvailability(0u, session.get()));
  EXPECT_TRUE(context.GetDohServerAvailability(1u, session.get()));

  std::unique_ptr<DnsServerIterator> doh_itr = context.GetDohIterator(
      session->config(), SecureDnsMode::kAutomatic, session.get());
  EXPECT_TRUE(doh_itr->AttemptAvailable());

  context.RecordServerFailure(1u /* server_index */, true /* is_doh_server */,
                              ERR_FAILED, session.get());
  EXPECT_EQ(context.NumAvailableDohServers(session.get()), 0u);
  EXPECT_FALSE(context.GetDohServerAvailability(0u, session.get()));
  EXPECT_FALSE(context.GetDohServerAvailability(1u, session.get()));
  EXPECT_FALSE(doh_itr->AttemptAvailable());
}

TEST_F(BraveResolveContextTest, DohServerAvailability_RecordServerSuccess) {
  scoped_refptr<DnsSession> session = CreateDnsSession(CreateDnsConfig());

  URLRequestContext request_context;
  BraveResolveContext context(&request_context, true /* enable_caching */);
  context.InvalidateCachesAndPerSessionData(session.get(),
                                            false /* network_change */);

  context.RecordServerSuccess(0u /* server_index */, true /* is_doh_server */,
                              session.get());
  EXPECT_EQ(context.NumAvailableDohServers(session.get()), 2u);
  EXPECT_TRUE(context.GetDohServerAvailability(0u, session.get()));
  EXPECT_TRUE(context.GetDohServerAvailability(1u, session.get()));

  std::unique_ptr<DnsServerIterator> doh_itr = context.GetDohIterator(
      session->config(), SecureDnsMode::kAutomatic, session.get());
  EXPECT_TRUE(doh_itr->AttemptAvailable());

  context.RecordServerSuccess(1u /* server_index */, true /* is_doh_server */,
                              session.get());
  EXPECT_EQ(context.NumAvailableDohServers(session.get()), 2u);
  EXPECT_TRUE(context.GetDohServerAvailability(0u, session.get()));
  EXPECT_TRUE(context.GetDohServerAvailability(1u, session.get()));
  EXPECT_TRUE(doh_itr->AttemptAvailable());
}

}  // namespace

}  // namespace net
