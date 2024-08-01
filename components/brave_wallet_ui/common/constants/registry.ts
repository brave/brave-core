// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../../constants/types'

export const SwapExchangeProxy = '0xdef1c0ded9bec7f1a1670819833240f027b25eff'
export const LiFiExchangeProxy = '0x1231deb6f5749ef6ce6943a275a1d3e7486f4eae'

export const BatRewardsContractAddress = BraveWallet.BAT_TOKEN_CONTRACT_ADDRESS

export default {
  [SwapExchangeProxy]: '0x Exchange Proxy',
  [LiFiExchangeProxy]: 'Li.Fi Exchange Proxy'
}
