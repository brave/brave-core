/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_constants.h"

namespace ipfs {

const char kSwarmPeersPath[] = "/api/v0/swarm/peers";
const char kConfigPath[] = "/api/v0/config";
const char kArgQueryParam[] = "arg";
const char kAddressesField[] = "Addresses";
const char kShutdownPath[] = "/api/v0/shutdown";
const char kIPFSScheme[] = "ipfs";
const char kIPNSScheme[] = "ipns";
const char kDefaultIPFSGateway[] = "https://dweb.link";
const char kDefaultIPFSLocalGateway[] = "http://localhost";
const char kIPFSLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/sections/"
    "360010974932-InterPlanetary-File-System-IPFS-";

}  // namespace ipfs
