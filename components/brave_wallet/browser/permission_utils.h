/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_PERMISSION_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_PERMISSION_UTILS_H_

#include <cstdint>
#include <optional>
#include <queue>
#include <string>
#include <string_view>
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
 * Parse the overwritten requesting origins from wallet permission
 * sub-requests, validate its format and extract original requesting_origin
 * and account address of one sub-request.
 * Ex: Given input origin as https://origin0x123..., it will return
 * https://origin as the original requesting_origin and 0x123... as the account
 * address.
 */
bool ParseRequestingOriginFromSubRequest(permissions::RequestType type,
                                         const url::Origin& origin,
                                         url::Origin* requesting_origin,
                                         std::string* account);

/**
 * Given old_origin, adding account info to its host part and return as
 * new_origin. If type != kBraveEthereum, there would be separator like
 * https://origin__BrG4...
 */
std::optional<url::Origin> GetSubRequestOrigin(permissions::RequestType type,
                                               const url::Origin& old_origin,
                                               std::string_view account);

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
