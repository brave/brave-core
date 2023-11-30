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
#endif

namespace {
const int kDefaultKuboAPIPort = 5001;
}  // namespace

namespace ipfs {

bool IpfsNodeTrafficRecognizer::IsKuboRelatedPort(
    const GURL& request_url,
    const version_info::Channel& channel) {
#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
  const std::string_view port = request_url.port_piece();
  if (port == ipfs::GetAPIPort(channel) ||
      port == base::NumberToString(kDefaultKuboAPIPort)) {
    return true;
  }
#endif
  return false;
}

bool IpfsNodeTrafficRecognizer::IsKuboRelatedDomain(const GURL& request_url) {
#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
  const url::Origin origin = url::Origin::Create(request_url);
  if (origin.DomainIs(ipfs::kLocalhostDomain) ||
      origin.DomainIs(ipfs::kLocalhostIP)) {
    return true;
  }
#endif
  return false;
}

}  // namespace ipfs
