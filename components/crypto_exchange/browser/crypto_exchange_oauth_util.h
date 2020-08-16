/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CRYPTO_EXCHANGE_BROWSER_CRYPTO_EXCHANGE_OAUTH_UTIL_H_
#define BRAVE_COMPONENTS_CRYPTO_EXCHANGE_BROWSER_CRYPTO_EXCHANGE_OAUTH_UTIL_H_

#include <string>

namespace crypto_exchange {

std::string GetCryptoRandomString(bool hex_encode);

std::string GetCodeChallenge(
    const std::string& code_verifier, bool strip_chars);

}

#endif  // BRAVE_COMPONENTS_CRYPTO_EXCHANGE_BROWSER_CRYPTO_EXCHANGE_OAUTH_UTIL_H_
