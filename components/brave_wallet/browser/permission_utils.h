/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_PERMISSION_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_PERMISSION_UTILS_H_

#include <optional>
#include <queue>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/permissions/request_type.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"

class GURL;
namespace url {
class Origin;
}

namespace brave_wallet {

/**
 * Add wallet addresses to the origin of the website asking wallet
 * permission in a format as https://old_origin{addr=address1&addr=address2}
 * and return it. Return origin if successful; return nullopt if caller passes
 * invalid old_origin or empty addresses.
 */
std::optional<url::Origin> GetConcatOriginFromWalletAddresses(
    const url::Origin& old_origin,
    const std::vector<std::string>& addresses);

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
 * Parse the overwritten requesting origins of wallet permission requests,
 * validate its format and extract original requesting_origin and addresses
 * included in the overwritten requesting origin.
 * Ex: Given input origin as https://origin{addr=0x123...&addr=0x456...}, it
 * will return https://origin as the original requesting_origin and
 * {0x123, 0x456} as the address_queue.
 */
bool ParseRequestingOrigin(permissions::RequestType type,
                           const url::Origin& origin,
                           url::Origin* requesting_origin,
                           std::queue<std::string>* address_queue);

/**
 * Given old_origin, adding account info to its host part and return as
 * new_origin. If type != kBraveEthereum, there would be separator like
 * https://origin__BrG4...
 */
std::optional<url::Origin> GetSubRequestOrigin(permissions::RequestType type,
                                               const url::Origin& old_origin,
                                               const std::string& account);

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
