// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// constants
import { BraveWallet } from '../../constants/types'

// utils
import Amount from '../../utils/amount'

// queries
import { useGetTokenInfoQuery } from '../slices/api.slice'
import { useGetCombinedTokensListQuery } from '../slices/api.slice.extra'

interface Arg {
  contractAddress: string
  tokenId?: string
  network: Pick<BraveWallet.NetworkInfo, 'chainId' | 'coin'>
}

export default function useGetTokenInfo(arg: Arg | typeof skipToken) {
  const { data: combinedTokensList } = useGetCombinedTokensListQuery()

  const tokenInfoFromTokensList = React.useMemo(() => {
    if (arg === skipToken) {
      return undefined
    }

    return combinedTokensList.find(
      (t) =>
        t.contractAddress.toLowerCase() === arg.contractAddress.toLowerCase() &&
        t.chainId === arg.network.chainId &&
        t.coin === arg.network.coin &&
        (!arg.tokenId || arg.tokenId === t.tokenId)
    )
  }, [combinedTokensList, arg])

  const { data: tokenInfoFromRpc, isFetching } = useGetTokenInfoQuery(
    arg !== skipToken &&
      arg.network &&
      arg.contractAddress &&
      !tokenInfoFromTokensList
      ? {
          coin: arg.network.coin,
          chainId: arg.network.chainId,
          contractAddress: arg.contractAddress
        }
      : skipToken
  )

  const nftFromRpc = React.useMemo(() => {
    return arg !== skipToken && tokenInfoFromRpc && arg?.tokenId
      ? { ...tokenInfoFromRpc, tokenId: new Amount(arg.tokenId).toHex() }
      : undefined
  }, [arg, tokenInfoFromRpc])

  return {
    isVisible: tokenInfoFromTokensList?.visible ?? false,
    tokenInfo:
      tokenInfoFromTokensList ?? nftFromRpc ?? tokenInfoFromRpc ?? undefined,
    isLoading: combinedTokensList.length === 0 || isFetching
  }
}
