// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { BraveWallet, WalletAccountType } from '../../constants/types'
import { getTokensCoinType } from '../../utils/network-utils'

export default function useBalance (networks: BraveWallet.NetworkInfo[]) {
  return React.useCallback((account?: WalletAccountType, token?: BraveWallet.BlockchainToken) => {
    if (!account || !token) {
      return ''
    }

    if (!account.tokenBalanceRegistry) {
      return ''
    }

    const tokensCoinType = getTokensCoinType(networks, token)
    // Return native asset balance
    if (
      (
        token.contractAddress === '' ||
        // 0xeee... is used as a contractAddress
        // of native assets in our Swap Widget and used for the 0x API.
        token.contractAddress === '0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee'
      ) &&
      // Since all coinTypes share the same chainId for localHost networks,
      // we want to make sure we return the right balance for that token.
      account.coin === tokensCoinType
    ) {
      return (account.nativeBalanceRegistry || {})[token.chainId || ''] || ''
    }

    return (account.tokenBalanceRegistry || {})[token.contractAddress.toLowerCase()] || ''
  }, [networks])
}
