/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_HELPER_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_vpn/mojom/brave_vpn.mojom.h"

namespace base {
class Value;
}  // namespace base

namespace brave_vpn {

struct Hostname;

bool ValidateCachedRegionData(const base::Value& region_value);
std::string GetBraveVPNPaymentsEnv(const std::string& env);

base::Value GetValueFromRegion(const mojom::Region& region);
std::unique_ptr<Hostname> PickBestHostname(
    const std::vector<Hostname>& hostnames);
std::vector<Hostname> ParseHostnames(const base::Value& hostnames);
std::vector<mojom::Region> ParseRegionList(const base::Value& region_list);
base::Value GetValueWithTicketInfos(const std::string& email,
                                    const std::string& subject,
                                    const std::string& body);
mojom::RegionPtr GetRegionPtrWithNameFromRegionList(
    const std::string& name,
    const std::vector<mojom::Region> region_list);

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_HELPER_H_
