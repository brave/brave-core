/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_RLP_DECODE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_RLP_DECODE_H_

#include <string>

#include "base/values.h"

namespace brave_wallet {

// Recursive Length Prefix (RLP) decoding of arbitrarily nested arrays of data
// Input string should be a hex string but without the 0x prefix
bool RLPDecode(const std::string& s, base::Value* output);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_RLP_DECODE_H_
