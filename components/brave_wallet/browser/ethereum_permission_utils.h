/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETHEREUM_PERMISSION_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETHEREUM_PERMISSION_UTILS_H_

#include <queue>
#include <string>
#include <vector>

class GURL;

namespace brave_wallet {

/**
 * Add wallet addresses to the origin of the website asking Ethereum
 * permission in a format as https://old_origin{addr=address1&addr=address2}
 * and return it. Return true if successful; return false if caller passes
 * invalid old_origin or empty addresses.
 */
bool GetConcatOriginFromWalletAddresses(
    const GURL& old_origin,
    const std::vector<std::string>& addresses,
    GURL* new_origin);

/**
 * Parse the overwritten requesting origins from ethereum permission
 * sub-requests, validate its format and extract original requesting_origin
 * and account address of one sub-request.
 * Ex: Given input origin as https://origin0x123..., it will return
 * https://origin as the original requesting_origin and 0x123... as the account
 * address.
 */
bool ParseRequestingOriginFromSubRequest(const GURL& origin,
                                         std::string* requesting_origin,
                                         std::string* account);

/**
 * Parse the overwritten requesting origins of ethereum permission requests,
 * validate its format and extract original requesting_origin and addresses
 * included in the overwritten requesting origin.
 * Ex: Given input origin as https://origin{addr=0x123...&addr=0x456...}, it
 * will return https://origin as the original requesting_origin and
 * {0x123, 0x456} as the address_queue.
 */
bool ParseRequestingOrigin(const GURL& origin,
                           std::string* requesting_origin,
                           std::queue<std::string>* address_queue);

/**
 * Given old_origin, adding account info to its host part and return as
 * new_origin.
 */
bool GetSubRequestOrigin(const GURL& old_origin,
                         const std::string& account,
                         GURL* new_origin);

/**
 * Given tab ID, accounts, and origin, return the WebUI URL for connecting
 * with site (ethereum permission) request.
 * Example output:
 *   chrome://wallet-panel.top-chrome/?addr=0x123&addr=0x456&tabId=1&origin=https://test.com
 */
GURL GetConnectWithSiteWebUIURL(const GURL& webui_base_url,
                                int32_t tab_id,
                                const std::vector<std::string>& accounts,
                                const std::string& origin);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETHEREUM_PERMISSION_UTILS_H_
