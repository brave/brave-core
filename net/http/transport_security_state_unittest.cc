/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/http/transport_security_state.h"

#include "base/test/scoped_feature_list.h"
#include "net/base/features.h"
#include "net/base/isolation_info.h"
#include "net/base/network_anonymization_key.h"
#include "net/base/schemeful_site.h"
#include "net/test/test_with_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/origin.h"

namespace net {

namespace {

constexpr char kHSTSHeaderValue[] = "max-age=600000";

}  // namespace

class TransportSecurityStateTestBase : public ::testing::Test,
                                       public WithTaskEnvironment {
 public:
  TransportSecurityStateTestBase()
      : WithTaskEnvironment(
            base::test::TaskEnvironment::TimeSource::MOCK_TIME) {
    // Need mocked out time for pruning tests. Don't start with a
    // time of 0, as code doesn't generally expect it.
    FastForwardBy(base::Days(1));
  }

  static IsolationInfo CreateIsolationInfo(
      const url::Origin& top_frame_origin,
      const SiteForCookies& site_for_cookies) {
    return IsolationInfo::Create(IsolationInfo::RequestType::kMainFrame,
                                 top_frame_origin, top_frame_origin,
                                 site_for_cookies);
  }

  static NetworkAnonymizationKey CreateNetworkAnonymizationKey(
      const url::Origin& top_frame_origin) {
    SchemefulSite schemeful_site(top_frame_origin);
    return net::NetworkAnonymizationKey::CreateFromFrameSite(schemeful_site,
                                                             schemeful_site);
  }

  void ExpectNoHSTS(TransportSecurityState* state,
                    const NetworkAnonymizationKey& network_anonymization_key,
                    const std::string& host) {
    SCOPED_TRACE(testing::Message() << network_anonymization_key.ToDebugString()
                                    << " host: " << host);
    EXPECT_FALSE(state->ShouldUpgradeToSSL(host));
    EXPECT_FALSE(state->ShouldUpgradeToSSL(network_anonymization_key, host));
    EXPECT_FALSE(state->ShouldSSLErrorsBeFatal(host));
    EXPECT_FALSE(
        state->ShouldSSLErrorsBeFatal(network_anonymization_key, host));
    TransportSecurityState::STSState dynamic_sts_state;
    EXPECT_FALSE(state->GetDynamicSTSState(host, &dynamic_sts_state));
  }

  void ExpectHasHSTS(TransportSecurityState* state,
                     const NetworkAnonymizationKey& network_anonymization_key,
                     const std::string& host) {
    SCOPED_TRACE(testing::Message() << network_anonymization_key.ToDebugString()
                                    << " host: " << host);
    EXPECT_TRUE(state->ShouldUpgradeToSSL(host));
    EXPECT_TRUE(state->ShouldUpgradeToSSL(network_anonymization_key, host));
    EXPECT_TRUE(state->ShouldSSLErrorsBeFatal(host));
    EXPECT_TRUE(state->ShouldSSLErrorsBeFatal(network_anonymization_key, host));
    {
      TransportSecurityState::STSState dynamic_sts_state;
      EXPECT_TRUE(state->GetDynamicSTSState(host, &dynamic_sts_state));
      EXPECT_EQ(dynamic_sts_state.upgrade_mode,
                TransportSecurityState::STSState::MODE_FORCE_HTTPS);
    }
  }

  void ExpectHasHSTSOnlyWithNIK(
      TransportSecurityState* state,
      const NetworkAnonymizationKey& network_anonymization_key,
      const std::string& host) {
    SCOPED_TRACE(testing::Message() << network_anonymization_key.ToDebugString()
                                    << " host: " << host);
    EXPECT_FALSE(state->ShouldUpgradeToSSL(host));
    EXPECT_TRUE(state->ShouldUpgradeToSSL(network_anonymization_key, host));
    EXPECT_FALSE(state->ShouldSSLErrorsBeFatal(host));
    EXPECT_TRUE(state->ShouldSSLErrorsBeFatal(network_anonymization_key, host));
    {
      TransportSecurityState::STSState dynamic_sts_state;
      EXPECT_FALSE(state->GetDynamicSTSState(host, &dynamic_sts_state));
    }
  }
};

class TransportSecurityState_DisableHSTSPartitionTest
    : public TransportSecurityStateTestBase {
 public:
  TransportSecurityState_DisableHSTSPartitionTest() {
    scoped_feature_list_.InitAndDisableFeature(features::kBravePartitionHSTS);
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(TransportSecurityState_DisableHSTSPartitionTest,
       UnpartitionedAddHSTSHeader) {
  TransportSecurityState state;

  auto a_com_origin = url::Origin::Create(GURL("https://a.com"));

  ExpectNoHSTS(&state, NetworkAnonymizationKey(), "a.com");
  ExpectNoHSTS(&state, NetworkAnonymizationKey(), "b.com");

  // Simulate valid IsolationInfo (it shouldn't be used anyways).
  EXPECT_TRUE(state.AddHSTSHeader(
      CreateIsolationInfo(a_com_origin,
                          SiteForCookies::FromOrigin(a_com_origin)),
      "a.com", kHSTSHeaderValue));
  // Simulate invalid IsolationInfo (it shouldn't be used anyways).
  EXPECT_TRUE(state.AddHSTSHeader(IsolationInfo(), "b.com", kHSTSHeaderValue));

  ExpectHasHSTS(&state, NetworkAnonymizationKey(), "a.com");
  ExpectHasHSTS(&state, NetworkAnonymizationKey(), "b.com");
}

TEST_F(TransportSecurityState_DisableHSTSPartitionTest,
       UnpartitionedDeleteDynamicDataForHost) {
  TransportSecurityState state;

  EXPECT_TRUE(state.AddHSTSHeader(IsolationInfo(), "a.com", kHSTSHeaderValue));
  EXPECT_TRUE(state.AddHSTSHeader(IsolationInfo(), "b.com", kHSTSHeaderValue));
  ExpectHasHSTS(&state, NetworkAnonymizationKey(), "a.com");
  ExpectHasHSTS(&state, NetworkAnonymizationKey(), "b.com");

  EXPECT_TRUE(state.DeleteDynamicDataForHost("a.com"));
  ExpectNoHSTS(&state, NetworkAnonymizationKey(), "a.com");
  ExpectHasHSTS(&state, NetworkAnonymizationKey(), "b.com");

  // Second time shouldn't delete anything.
  EXPECT_FALSE(state.DeleteDynamicDataForHost("a.com"));
  ExpectHasHSTS(&state, NetworkAnonymizationKey(), "b.com");
}

class TransportSecurityState_EnableHSTSPartitionTest
    : public TransportSecurityStateTestBase {
 public:
  TransportSecurityState_EnableHSTSPartitionTest() {
    scoped_feature_list_.InitAndEnableFeature(features::kBravePartitionHSTS);
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(TransportSecurityState_EnableHSTSPartitionTest,
       PartitionedAddHSTSHeader) {
  base::test::ScopedFeatureList scoped_feature_list(
      features::kBravePartitionHSTS);
  TransportSecurityState state;

  auto a_com_origin = url::Origin::Create(GURL("https://a.com"));
  auto b_com_origin = url::Origin::Create(GURL("https://b.com"));

  ExpectNoHSTS(&state, NetworkAnonymizationKey(), "a.com");
  ExpectNoHSTS(&state, NetworkAnonymizationKey(), "bbb.com");

  // Add a.com record on a.com frame.
  EXPECT_TRUE(state.AddHSTSHeader(
      CreateIsolationInfo(a_com_origin,
                          SiteForCookies::FromOrigin(a_com_origin)),
      "a.com", kHSTSHeaderValue));
  // Try to add b.com on invalid partition.
  EXPECT_FALSE(state.AddHSTSHeader(IsolationInfo(), "b.com", kHSTSHeaderValue));
  // Add a.com on b.com frame.
  EXPECT_TRUE(state.AddHSTSHeader(
      CreateIsolationInfo(b_com_origin,
                          SiteForCookies::FromOrigin(b_com_origin)),
      "a.com", kHSTSHeaderValue));
  // Add bbb.com on b.com frame.
  EXPECT_TRUE(state.AddHSTSHeader(
      CreateIsolationInfo(b_com_origin,
                          SiteForCookies::FromOrigin(b_com_origin)),
      "bbb.com", kHSTSHeaderValue));

  ExpectHasHSTS(&state, CreateNetworkAnonymizationKey(a_com_origin), "a.com");
  ExpectNoHSTS(&state, NetworkAnonymizationKey(), "b.com");
  ExpectNoHSTS(&state, CreateNetworkAnonymizationKey(b_com_origin), "b.com");
  // Partitioned values should be available on b.com frame.
  ExpectHasHSTS(&state, CreateNetworkAnonymizationKey(b_com_origin), "a.com");
  ExpectHasHSTSOnlyWithNIK(&state, CreateNetworkAnonymizationKey(b_com_origin),
                           "bbb.com");
}

TEST_F(TransportSecurityState_EnableHSTSPartitionTest,
       PartitionedSaveAllHSTSOnHTTP) {
  base::test::ScopedFeatureList scoped_feature_list(
      features::kBravePartitionHSTS);
  TransportSecurityState state;

  auto a_com_origin = url::Origin::Create(GURL("http://a.com"));

  EXPECT_TRUE(state.AddHSTSHeader(
      CreateIsolationInfo(a_com_origin,
                          SiteForCookies::FromOrigin(a_com_origin)),
      "a.com", kHSTSHeaderValue));
  EXPECT_TRUE(
      state.AddHSTSHeader(CreateIsolationInfo(a_com_origin, SiteForCookies()),
                          "b.com", kHSTSHeaderValue));

  ExpectHasHSTS(&state, CreateNetworkAnonymizationKey(a_com_origin), "a.com");
  ExpectHasHSTSOnlyWithNIK(&state, CreateNetworkAnonymizationKey(a_com_origin),
                           "b.com");
}

TEST_F(TransportSecurityState_EnableHSTSPartitionTest,
       PartitionedSaveHSTSForOnlyMatchedSameSiteForCookiesOnHTTPS) {
  base::test::ScopedFeatureList scoped_feature_list(
      features::kBravePartitionHSTS);
  TransportSecurityState state;

  auto a_com_origin = url::Origin::Create(GURL("https://a.com"));

  EXPECT_TRUE(state.AddHSTSHeader(
      CreateIsolationInfo(a_com_origin,
                          SiteForCookies::FromOrigin(a_com_origin)),
      "a.com", kHSTSHeaderValue));
  EXPECT_FALSE(
      state.AddHSTSHeader(CreateIsolationInfo(a_com_origin, SiteForCookies()),
                          "b.com", kHSTSHeaderValue));

  ExpectHasHSTS(&state, CreateNetworkAnonymizationKey(a_com_origin), "a.com");
  ExpectNoHSTS(&state, CreateNetworkAnonymizationKey(a_com_origin), "b.com");
}

TEST_F(TransportSecurityState_EnableHSTSPartitionTest,
       PartitionedDeleteDynamicDataForHost) {
  TransportSecurityState state;

  auto a_com_origin = url::Origin::Create(GURL("https://a.com"));
  auto b_com_origin = url::Origin::Create(GURL("https://b.com"));
  auto c_com_origin = url::Origin::Create(GURL("https://c.com"));

  EXPECT_TRUE(state.AddHSTSHeader(
      CreateIsolationInfo(b_com_origin,
                          SiteForCookies::FromOrigin(b_com_origin)),
      "a.com", kHSTSHeaderValue));
  EXPECT_TRUE(state.AddHSTSHeader(
      CreateIsolationInfo(c_com_origin,
                          SiteForCookies::FromOrigin(c_com_origin)),
      "a.com", kHSTSHeaderValue));
  ExpectNoHSTS(&state, NetworkAnonymizationKey(), "a.com");
  ExpectNoHSTS(&state, CreateNetworkAnonymizationKey(a_com_origin), "a.com");

  ExpectHasHSTSOnlyWithNIK(&state, CreateNetworkAnonymizationKey(b_com_origin),
                           "a.com");
  ExpectHasHSTSOnlyWithNIK(&state, CreateNetworkAnonymizationKey(c_com_origin),
                           "a.com");

  EXPECT_TRUE(state.AddHSTSHeader(
      CreateIsolationInfo(a_com_origin,
                          SiteForCookies::FromOrigin(a_com_origin)),
      "a.com", kHSTSHeaderValue));
  ExpectHasHSTS(&state, CreateNetworkAnonymizationKey(a_com_origin), "a.com");

  EXPECT_TRUE(state.AddHSTSHeader(
      CreateIsolationInfo(b_com_origin,
                          SiteForCookies::FromOrigin(b_com_origin)),
      "b.com", kHSTSHeaderValue));
  ExpectHasHSTS(&state, CreateNetworkAnonymizationKey(b_com_origin), "b.com");

  EXPECT_TRUE(state.DeleteDynamicDataForHost("a.com"));
  ExpectNoHSTS(&state, CreateNetworkAnonymizationKey(a_com_origin), "a.com");
  ExpectNoHSTS(&state, CreateNetworkAnonymizationKey(b_com_origin), "a.com");
  ExpectNoHSTS(&state, CreateNetworkAnonymizationKey(c_com_origin), "a.com");
  ExpectHasHSTS(&state, CreateNetworkAnonymizationKey(b_com_origin), "b.com");

  // Second time shouldn't delete anything.
  EXPECT_FALSE(state.DeleteDynamicDataForHost("a.com"));
  ExpectHasHSTS(&state, CreateNetworkAnonymizationKey(b_com_origin), "b.com");
}

}  // namespace net
