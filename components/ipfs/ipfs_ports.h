/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPFS_PORTS_H_
#define BRAVE_COMPONENTS_IPFS_IPFS_PORTS_H_

#include <string>

#include "components/version_info/channel.h"

namespace ipfs {

std::string GetAPIPort(version_info::Channel channel);
std::string GetGatewayPort(version_info::Channel channel);
std::string GetSwarmPort(version_info::Channel channel);

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IPFS_PORTS_H_
