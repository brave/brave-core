/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_node_traffic_recognizer.h"

#include <string>

#include "base/strings/strcat.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_ports.h"
#include "components/version_info/channel.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {
inline constexpr version_info::Channel kChannelsToEnumerate[] = {
    version_info::Channel::UNKNOWN, version_info::Channel::CANARY,
    version_info::Channel::DEV, version_info::Channel::BETA,
    version_info::Channel::STABLE};

std::string ConstructTestUrl(const std::string& host, const std::string& port) {
  return base::StrCat({"http://", host, ":", port.c_str(), "/api/v0/version"});
}
}  // namespace

TEST(IPFSNodeTrafficRecognizerTest, RecognizeKuboRpcRequests) {
  ipfs::IpfsNodeTrafficRecognizer recognizer;
  GURL request_url_ip_def_api_port(
      ConstructTestUrl(ipfs::kLocalhostIP, "5001"));
  ASSERT_TRUE(recognizer.IsKuboRelatedUrl(request_url_ip_def_api_port));
  GURL request_url_lh_def_api_port(
      ConstructTestUrl(ipfs::kLocalhostDomain, "5001"));
  ASSERT_TRUE(recognizer.IsKuboRelatedUrl(request_url_lh_def_api_port));
  GURL non_kubo_port_request_url_ip(
      ConstructTestUrl(ipfs::kLocalhostIP, "7788"));
  ASSERT_FALSE(recognizer.IsKuboRelatedUrl(non_kubo_port_request_url_ip));
  GURL non_kubo_port_request_url_lh(
      ConstructTestUrl(ipfs::kLocalhostDomain, "7788"));
  ASSERT_FALSE(recognizer.IsKuboRelatedUrl(non_kubo_port_request_url_lh));

  for (const auto& channel : kChannelsToEnumerate) {
    auto kubo_port = ipfs::GetAPIPort(channel);
    GURL request_url_localhost(
        ConstructTestUrl(ipfs::kLocalhostDomain, kubo_port));
    ASSERT_TRUE(recognizer.IsKuboRelatedUrl(request_url_localhost));
    GURL request_url_localhostip(
        ConstructTestUrl(ipfs::kLocalhostIP, kubo_port));
    ASSERT_TRUE(recognizer.IsKuboRelatedUrl(request_url_localhostip));
    GURL non_kubo_host_request_url(ConstructTestUrl("somehost", kubo_port));
    ASSERT_FALSE(recognizer.IsKuboRelatedUrl(non_kubo_host_request_url));
  }
}
