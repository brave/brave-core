/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_ports.h"

#include "base/strings/string_number_conversions.h"

namespace {

// Default ports are used for unknown channel, other channels use ports with an
// channel offset to these values.
const int kDefaultAPIPort = 45001;
const int kDefaultGatewayPort = 48080;
const int kDefaultSwarmPort = 44001;

}  // namespace

namespace ipfs {

std::string GetAPIPort(version_info::Channel channel) {
  return base::NumberToString(kDefaultAPIPort + static_cast<int>(channel));
}

std::string GetGatewayPort(version_info::Channel channel) {
  return base::NumberToString(kDefaultGatewayPort + static_cast<int>(channel));
}

std::string GetSwarmPort(version_info::Channel channel) {
  return base::NumberToString(kDefaultSwarmPort + static_cast<int>(channel));
}

}  // namespace ipfs
