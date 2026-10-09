/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_PERMISSION_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_PERMISSION_UTILS_H_

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/common/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_BRAVE_WALLET));
class GURL;
namespace url {
class Origin;
}

namespace blink {
enum class PermissionType;
}

namespace permissions {
enum class RequestType;
}

namespace brave_wallet {
namespace mojom {
enum class CoinType : int32_t;
}

/**
 * Given accounts, and origin, return the WebUI URL for connecting with site
 * (ethereum permission) request.
 * Example output:
 *   chrome://wallet-panel.top-chrome/?addr=0x123&addr=0x456&origin=https://test.com
 */
GURL GetConnectWithSiteWebUIURL(const GURL& webui_base_url,
                                const std::vector<std::string>& accounts,
                                const url::Origin& origin);

std::optional<blink::PermissionType> CoinTypeToPermissionType(
    mojom::CoinType coin_type);

std::optional<permissions::RequestType> CoinTypeToPermissionRequestType(
    mojom::CoinType coin_type);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_PERMISSION_UTILS_H_
