/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/components/unstoppable_domains/constants.h"
#include "brave/components/unstoppable_domains/features.h"

#include "../../../../net/dns/dns_transaction_unittest.cc"

namespace {

static const char kTestCryptoHostName[] = "test.crypto";

// Response contains IP address: 142.250.72.196 for test.crypto.
static const uint8_t kTestCryptoResponseDatagram[] = {
    0x00, 0x00, 0x81, 0x80, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x04, 0x74, 0x65, 0x73, 0x74, 0x06, 0x63, 0x72, 0x79, 0x70, 0x74, 0x6f,
    0x00, 0x00, 0x01, 0x00, 0x01, 0xc0, 0x0c, 0x00, 0x01, 0x00, 0x01, 0x00,
    0x00, 0x00, 0xa2, 0x00, 0x04, 0x8e, 0xfa, 0x48, 0xc4};

}  // namespace

namespace net {

class BraveDnsTransactionTest : public DnsTransactionTestBase,
                                public WithTaskEnvironment {
 public:
  BraveDnsTransactionTest() {
    feature_list_.InitAndEnableFeature(
        unstoppable_domains::features::kUnstoppableDomains);
  }

  ~BraveDnsTransactionTest() override = default;

  void BraveConfigureDohServers(bool multiple_server) {
    GURL url(unstoppable_domains::kDoHResolver);
    URLRequestFilter* filter = URLRequestFilter::GetInstance();
    filter->AddHostnameInterceptor(url.scheme(), url.host(),
                                   std::make_unique<DohJobInterceptor>(this));
    config_.dns_over_https_servers.push_back(
        {unstoppable_domains::kDoHResolver, true});

    if (multiple_server) {
      GURL url2("https://test.com/dns-query");
      filter->AddHostnameInterceptor(url2.scheme(), url2.host(),
                                     std::make_unique<DohJobInterceptor>(this));
      config_.dns_over_https_servers.push_back({url2.spec(), true});
    }

    ConfigureFactory();
    for (size_t server_index = 0;
         server_index < config_.dns_over_https_servers.size(); ++server_index) {
      resolve_context_->RecordServerSuccess(
          server_index, true /* is_doh_server */, session_.get());
    }
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(BraveDnsTransactionTest, SkipUDResolverForNonCryptoDomainsSingleServer) {
  BraveConfigureDohServers(false);
  EXPECT_TRUE(resolve_context_->GetDohServerAvailability(
      0u /* doh_server_index */, session_.get()));
  TransactionHelper helper0(ERR_BLOCKED_BY_CLIENT);
  helper0.StartTransaction(transaction_factory_.get(), kT0HostName, kT0Qtype,
                           true /* secure */, resolve_context_.get());
  helper0.RunUntilComplete();
}

TEST_F(BraveDnsTransactionTest,
       SkipUDResolverForNonCryptoDomainsMultipleServers) {
  BraveConfigureDohServers(true);
  AddQueryAndResponse(0, kT0HostName, kT0Qtype, kT0ResponseDatagram,
                      base::size(kT0ResponseDatagram), SYNCHRONOUS,
                      Transport::HTTPS, nullptr /* opt_rdata */,
                      DnsQuery::PaddingStrategy::BLOCK_LENGTH_128,
                      false /* enqueue_transaction_id */);
  TransactionHelper helper0(kT0RecordCount);
  helper0.StartTransaction(transaction_factory_.get(), kT0HostName, kT0Qtype,
                           true /* secure */, resolve_context_.get());
  helper0.RunUntilComplete();
}

TEST_F(BraveDnsTransactionTest, UseUDResolverForCryptoDomainsSingleServer) {
  BraveConfigureDohServers(false);
  AddQueryAndResponse(
      0, kTestCryptoHostName, dns_protocol::kTypeA, kTestCryptoResponseDatagram,
      base::size(kTestCryptoResponseDatagram), SYNCHRONOUS, Transport::HTTPS,
      nullptr /* opt_rdata */, DnsQuery::PaddingStrategy::BLOCK_LENGTH_128,
      false /* enqueue_transaction_id */);
  TransactionHelper helper0(1);
  helper0.StartTransaction(transaction_factory_.get(), kTestCryptoHostName,
                           dns_protocol::kTypeA, true /* secure */,
                           resolve_context_.get());
  helper0.RunUntilComplete();
}

TEST_F(BraveDnsTransactionTest, UseUDResolverForCryptoDomainsMultipleServer) {
  BraveConfigureDohServers(true);
  AddQueryAndResponse(
      0, kTestCryptoHostName, dns_protocol::kTypeA, kTestCryptoResponseDatagram,
      base::size(kTestCryptoResponseDatagram), SYNCHRONOUS, Transport::HTTPS,
      nullptr /* opt_rdata */, DnsQuery::PaddingStrategy::BLOCK_LENGTH_128,
      false /* enqueue_transaction_id */);
  TransactionHelper helper0(1);
  helper0.StartTransaction(transaction_factory_.get(), kTestCryptoHostName,
                           dns_protocol::kTypeA, true /* secure */,
                           resolve_context_.get());
  helper0.RunUntilComplete();
}

}  // namespace net
