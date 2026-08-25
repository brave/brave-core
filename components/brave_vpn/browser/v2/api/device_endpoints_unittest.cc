/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/api/device_endpoints.h"

#include "base/values.h"
#include "brave/components/brave_vpn/browser/v2/api/transport_protocol.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2::endpoints {
namespace {
constexpr char kTestSubscriberCredential[] = "test-subscriber-credential";
constexpr char kTestPublicKey[] = "test-public-key";
constexpr char kTestMultihopExitRegion[] = "us-west";
constexpr char kTestApiAuthToken[] = "test-api-auth-token";
}  // namespace

TEST(DeviceEndpointsTest, TransportProtocolToStringConversion) {
  EXPECT_EQ(ToTransportProtocolString(TransportProtocol::kIKEv2), "ikev2");
  EXPECT_EQ(ToTransportProtocolString(TransportProtocol::kWireguard),
            "wireguard");
}

TEST(DeviceEndpointsTest, GetProfileCredentialsRequestBodyToValueIkev2) {
  // IKEv2 never sends "public-key", even if explicitly set.
  const GetProfileCredentialsRequestBody body{
      .subscriber_credential = kTestSubscriberCredential,
      .transport_protocol = TransportProtocol::kIKEv2,
      .public_key = kTestPublicKey};
  EXPECT_EQ(body.ToValue(),
            base::DictValue()
                .Set("subscriber-credential", kTestSubscriberCredential)
                .Set("transport-protocol", "ikev2"));
}

TEST(DeviceEndpointsTest, GetProfileCredentialsRequestBodyToValueWireguard) {
  const GetProfileCredentialsRequestBody body{
      .subscriber_credential = kTestSubscriberCredential,
      .transport_protocol = TransportProtocol::kWireguard,
      .public_key = kTestPublicKey};
  EXPECT_EQ(body.ToValue(),
            base::DictValue()
                .Set("subscriber-credential", kTestSubscriberCredential)
                .Set("transport-protocol", "wireguard")
                .Set("public-key", kTestPublicKey));
}

TEST(DeviceEndpointsTest,
     GetProfileCredentialsRequestBodyToValueIncludesMultihopExitRegionWhenSet) {
  const GetProfileCredentialsRequestBody body{
      .subscriber_credential = kTestSubscriberCredential,
      .transport_protocol = TransportProtocol::kIKEv2,
      .multihop_exit_region = kTestMultihopExitRegion};
  EXPECT_EQ(body.ToValue(),
            base::DictValue()
                .Set("subscriber-credential", kTestSubscriberCredential)
                .Set("transport-protocol", "ikev2")
                .Set("multihop-exit-region", kTestMultihopExitRegion));
}

TEST(DeviceEndpointsTest, InvalidateCredentialsRequestBodyToValue) {
  const InvalidateCredentialsRequestBody body{
      .api_auth_token = kTestApiAuthToken,
      .subscriber_credential = kTestSubscriberCredential};
  EXPECT_EQ(body.ToValue(),
            base::DictValue()
                .Set("api-auth-token", kTestApiAuthToken)
                .Set("subscriber-credential", kTestSubscriberCredential));
}

TEST(DeviceEndpointsTest, SetMultihopExitRegionRequestBodyToValue) {
  const SetMultihopExitRegionRequestBody body{
      .api_auth_token = kTestApiAuthToken,
      .multihop_exit_region = kTestMultihopExitRegion};
  EXPECT_EQ(body.ToValue(),
            base::DictValue()
                .Set("api-auth-token", kTestApiAuthToken)
                .Set("multihop-exit-region", kTestMultihopExitRegion));
}

}  // namespace brave_vpn::v2::endpoints
