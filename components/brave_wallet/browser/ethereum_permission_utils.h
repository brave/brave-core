/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETHEREUM_PERMISSION_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETHEREUM_PERMISSION_UTILS_H_

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
 * Parse the overwritten requesting origins of ethereum permission requests,
 * validate its format and extract original requesting_origin and account
 * address of one sub-request.
 * sub_req_format: https://origin0x123... -> return https://origin as the
 * original requesting_origin and 0x123... as the account address.
 * non_sub_req_format: https://origin{addr=0x123...&addr=0x456...} -> return
 * https://origin as the original requesting_origin.
 */
bool ParseRequestingOrigin(const GURL& origin,
                           bool sub_req_format,
                           std::string* requesting_origin,
                           std::string* account);

/**
 * Given old_origin, adding account info to its host part and return as
 * new_origin.
 */
bool GetSubRequestOrigin(const GURL& old_origin,
                         const std::string& account,
                         GURL* new_origin);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETHEREUM_PERMISSION_UTILS_H_
