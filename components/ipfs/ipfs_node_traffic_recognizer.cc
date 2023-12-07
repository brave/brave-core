/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_node_traffic_recognizer.h"

#include "brave/components/ipfs/buildflags/buildflags.h"
#include "components/version_info/channel.h"

#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_ports.h"
#include "url/origin.h"

namespace {
inline constexpr int kDefaultKuboApiPort = 5001;

inline constexpr version_info::Channel kChannelsToEnumerate[] = {
    version_info::Channel::UNKNOWN, version_info::Channel::CANARY,
    version_info::Channel::DEV, version_info::Channel::BETA,
    version_info::Channel::STABLE};

bool IsKuboDomain(const url::Origin& origin) {
  return origin.DomainIs(ipfs::kLocalhostDomain) ||
         origin.DomainIs(ipfs::kLocalhostIP);
}
}  // namespace
#endif  // BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)

namespace ipfs {

bool IpfsNodeTrafficRecognizer::IsKuboRelatedUrl(const GURL& request_url) {
#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
  const url::Origin origin = url::Origin::Create(request_url);
  const std::string_view port = request_url.port_piece();

  if (IsKuboDomain(origin) &&
      port == base::NumberToString(kDefaultKuboApiPort)) {
    return true;
  }

  for (const auto& channel : kChannelsToEnumerate) {
    if (IsKuboDomain(origin) && port == ipfs::GetAPIPort(channel)) {
      return true;
    }
  }
#endif  // BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
  return false;
}

}  // namespace ipfs
