// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// types
import { BraveWallet } from '../../constants/types'

// utils
import { getAssetSymbol } from '../../utils/meld_utils'

// hooks
import { useGetMeldCryptoCurrenciesQuery } from '../slices/api.slice'

export const useFindBuySupportedToken = (
  token?: Pick<
    BraveWallet.BlockchainToken,
    'symbol' | 'contractAddress' | 'chainId'
  >
) => {
  // queries
  const { data: options } = useGetMeldCryptoCurrenciesQuery()

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
      foundNativeToken || foundTokenByContractAddress || foundTokenBySymbol
  }
}
