// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { skipToken } from '@reduxjs/toolkit/query/react'

// types
import { BraveWallet } from '../../constants/types'

// utils
import { getAssetSymbol } from '../../utils/meld_utils'
import { loadTimeData } from '../../../common/loadTimeData'

// hooks
import {
  useGetMeldCryptoCurrenciesQuery,
  useGetOnRampAssetsQuery
} from '../slices/api.slice'

export const useFindBuySupportedToken = (
  token?: Pick<
    BraveWallet.BlockchainToken,
    'symbol' | 'contractAddress' | 'chainId'
  >
) => {
  // Computed
  const isAndroid = loadTimeData.getBoolean('isAndroid') || false

  // queries
  const { data: options } = useGetMeldCryptoCurrenciesQuery(
    token && !isAndroid ? undefined : skipToken
  )

  const { data: androidOptions = undefined } = useGetOnRampAssetsQuery(
    token && isAndroid ? undefined : skipToken
  )

  const canBuyOnAndroid =
    token &&
    isAndroid &&
    androidOptions?.allAssetOptions.some(
      (asset) => asset.symbol.toLowerCase() === token.symbol.toLowerCase()
    )

  // computed
  const foundNativeToken =
    token &&
    token.contractAddress === '' &&
    options?.find(
      (asset) =>
        asset.chainId?.toLowerCase() === token.chainId.toLowerCase() &&
        getAssetSymbol(asset).toLowerCase() === token.symbol.toLowerCase()
    )

  const foundTokenByContractAddress =
    token &&
    options?.find(
      (asset) =>
        asset.contractAddress?.toLowerCase() ===
          token.contractAddress.toLowerCase() &&
        asset.chainId?.toLowerCase() === token.chainId.toLowerCase()
    )

  const foundTokenBySymbol =
    token &&
    options?.find(
      (asset) =>
        getAssetSymbol(asset).toLowerCase() === token.symbol.toLowerCase()
    )

  // render
  return {
    foundMeldBuyToken:
      foundNativeToken || foundTokenByContractAddress || foundTokenBySymbol,
    foundAndroidBuyToken: canBuyOnAndroid ? token : undefined
  }
}
