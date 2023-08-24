// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.


// you can obtain one at https://mozilla.org/MPL/2.0/.
export const MAX_UINT256 = '0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff'

/**
 * Blowfish's value for "Unlimited" is lower than our known max (MAX_UINT256)
 */
export const BLOWFISH_UNLIMITED_VALUE = '0xffffffffffffffffffffffffffffffffffffffffffffffffe43e9298b137ffff'

// 0x API expects native assets to have the following contract address.
export const NATIVE_ASSET_CONTRACT_ADDRESS_0X = '0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee'

export const WRAPPED_SOL_CONTRACT_ADDRESS = 'So11111111111111111111111111111111111111112'

/**
 * Set this as a token's coingecko id to flag the token as "unknown"
 * This will allow for fallback UI when we do have all the required token info
 */
export const UNKNOWN_TOKEN_COINGECKO_ID = 'UNKNOWN_TOKEN'

/**
 * use this ID to prevent looking up a token's price on coingecko
 */
export const SKIP_PRICE_LOOKUP_COINGECKO_ID ='skip-coingecko-price'

/**
 * Fiat value threshold for hidding small balances on the portolio view.
 * This may turn into a pref in the future.
 */
export const HIDE_SMALL_BALANCES_FIAT_THRESHOLD = 1
