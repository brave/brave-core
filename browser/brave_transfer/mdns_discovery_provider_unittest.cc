// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_transfer/mdns_discovery_provider.h"

#include <optional>
#include <string>
#include <vector>

#include "base/time/time.h"
#include "brave/components/brave_transfer/core/trust_keys.h"
#include "net/base/host_port_pair.h"
#include "net/base/ip_address.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_transfer {

namespace {

ChainKeys KeysFromByte(uint8_t value) {
  return *ChainKeys::FromSeed(std::vector<uint8_t>(32, value));
}

std::vector<std::string> TxtStrings(const ChainKeys& keys, base::Time now) {
  std::vector<std::string> strings;
  for (const auto& [key, value] : keys.TxtRecord(now)) {
    strings.push_back(key + "=" + value);
  }
  return strings;
}

local_discovery::ServiceDescription Service(std::vector<std::string> metadata) {
  local_discovery::ServiceDescription service;
  service.service_name = "a1b2c3._brave-xfer._tcp.local";
  service.address = net::HostPortPair("iPhone.local", 52000);
  EXPECT_TRUE(service.ip_address.AssignFromIPLiteral("192.168.1.20"));
  service.metadata = std::move(metadata);
  return service;
}

}  // namespace

TEST(BraveTransferMdnsDiscoveryTest, ParseTxtRecord) {
  const auto record =
      ParseTxtRecord({"v=1", "c=00ff", "flag", "=empty", "v=2", "x="});
  EXPECT_EQ(record.size(), 3u);
  EXPECT_EQ(record.at("v"), "1");
  EXPECT_EQ(record.at("c"), "00ff");
  EXPECT_EQ(record.at("x"), "");
  EXPECT_FALSE(record.contains("flag"));
}

TEST(BraveTransferMdnsDiscoveryTest, MatchingServiceBecomesCandidate) {
  const ChainKeys keys = KeysFromByte(3);
  const base::Time now = base::Time::Now();
  std::optional<PeerCandidate> candidate =
      CandidateFromService(Service(TxtStrings(keys, now)), keys, now);
  ASSERT_TRUE(candidate);
  EXPECT_EQ(candidate->provider, "mdns");
  EXPECT_EQ(candidate->instance_id, "a1b2c3._brave-xfer._tcp.local");
  EXPECT_EQ(candidate->tag, keys.TagForEpoch(EpochForTime(now)));
  ASSERT_EQ(candidate->endpoints.size(), 2u);
  EXPECT_EQ(candidate->endpoints[0], (PeerEndpoint{"192.168.1.20", 52000}));
  EXPECT_EQ(candidate->endpoints[1], (PeerEndpoint{"iPhone.local", 52000}));
}

TEST(BraveTransferMdnsDiscoveryTest, AdjacentEpochIsAccepted) {
  const ChainKeys keys = KeysFromByte(3);
  const base::Time now = base::Time::Now();
  EXPECT_TRUE(CandidateFromService(
      Service(TxtStrings(keys, now - base::Seconds(kEpochSeconds))), keys,
      now));
}

TEST(BraveTransferMdnsDiscoveryTest, OtherServicesAreIgnored) {
  const ChainKeys keys = KeysFromByte(3);
  const base::Time now = base::Time::Now();
  EXPECT_FALSE(CandidateFromService(Service(TxtStrings(KeysFromByte(4), now)),
                                    keys, now));
  EXPECT_FALSE(CandidateFromService(
      Service(TxtStrings(keys, now - base::Seconds(3 * kEpochSeconds))), keys,
      now));
  EXPECT_FALSE(CandidateFromService(Service({}), keys, now));

  std::vector<std::string> wrong_version;
  for (const std::string& entry : TxtStrings(keys, now)) {
    wrong_version.push_back(entry == "v=1" ? "v=2" : entry);
  }
  EXPECT_FALSE(CandidateFromService(Service(wrong_version), keys, now));

  local_discovery::ServiceDescription no_port = Service(TxtStrings(keys, now));
  no_port.address = net::HostPortPair("iPhone.local", 0);
  EXPECT_FALSE(CandidateFromService(no_port, keys, now));
}

TEST(BraveTransferMdnsDiscoveryTest, AddressWithoutResolvedIp) {
  const ChainKeys keys = KeysFromByte(3);
  const base::Time now = base::Time::Now();
  local_discovery::ServiceDescription service = Service(TxtStrings(keys, now));
  service.ip_address = net::IPAddress();
  service.address = net::HostPortPair("192.168.1.30", 52001);
  std::optional<PeerCandidate> candidate =
      CandidateFromService(service, keys, now);
  ASSERT_TRUE(candidate);
  ASSERT_EQ(candidate->endpoints.size(), 1u);
  EXPECT_EQ(candidate->endpoints[0], (PeerEndpoint{"192.168.1.30", 52001}));
}

}  // namespace brave_transfer
