/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_BRAVE_VPN_REGION_DATA_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_BRAVE_VPN_REGION_DATA_HELPER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"

namespace brave_vpn {
mojom::RegionPtr GetRegionPtrWithNameFromRegionList(
    const std::string& name,
    const std::vector<mojom::RegionPtr>& region_list);
base::Value::Dict GetValueFromRegionWithoutCity(const mojom::RegionPtr& region);
base::Value::Dict GetValueFromRegion(const mojom::RegionPtr& region);
bool IsValidRegionValue(const base::Value::Dict& value);
mojom::RegionPtr GetRegionFromValueWithoutCity(const base::Value::Dict& value);
mojom::RegionPtr GetRegionFromValue(const base::Value::Dict& value);
bool ValidateCachedRegionData(const base::Value::List& region_value);
std::vector<mojom::RegionPtr> ParseRegionList(
    const base::Value::List& region_list);

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_BRAVE_VPN_REGION_DATA_HELPER_H_
