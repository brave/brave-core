// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { BraveWallet, WalletAccountType, GetFlattenedAccountBalancesReturnInfo } from '../constants/types'

export type GetTokenParamArg = Pick<
  BraveWallet.BlockchainToken,
  | 'chainId'
  | 'contractAddress'
  | 'symbol'
> & {
  coingeckoId?: string | undefined
}

export const getTokenParam = (token: GetTokenParamArg): string => {
  if (token?.coingeckoId) {
    return token.coingeckoId
  }

  const isEthereumNetwork = token.chainId === BraveWallet.MAINNET_CHAIN_ID

  if (
    !isEthereumNetwork ||
    token.contractAddress === ''
  ) {
    return token.symbol.toLowerCase()
  }

  return token.contractAddress
}

// This will get the sum balance for each token between all accounts
export const getFlattenedAccountBalances = (accounts: WalletAccountType[], userVisibleTokensInfo: BraveWallet.BlockchainToken[]): GetFlattenedAccountBalancesReturnInfo[] => {
  if (accounts.length === 0) {
    return []
  }

  return userVisibleTokensInfo.map((token) => {
    return {
      token: token,
      balance: accounts
        .map(account => {
          const balance = token.contractAddress
            ? account.tokenBalanceRegistry[token.contractAddress.toLowerCase()]
            : account.nativeBalanceRegistry[token.chainId]

          return balance || '0'
        })
        .reduce((a, b) => a + Number(b), 0)
    }
  })
}
