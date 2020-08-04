/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CRYPTO_EXCHANGE_BROWSER_CRYPTO_EXCHANGE_REGION_UTIL_H_
#define BRAVE_COMPONENTS_CRYPTO_EXCHANGE_BROWSER_CRYPTO_EXCHANGE_REGION_UTIL_H_

#include <string>
#include <vector>

class PrefService;

namespace crypto_exchange {

bool IsRegionSupported(PrefService* pref_service,
    std::vector<std::string> regions,
    bool allow_list);

}  // namespace crypto_exchange

#endif  // BRAVE_COMPONENTS_CRYPTO_EXCHANGE_BROWSER_CRYPTO_EXCHANGE_REGION_UTIL_H_
