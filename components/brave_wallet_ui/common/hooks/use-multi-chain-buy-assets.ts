// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { skipToken } from '@reduxjs/toolkit/query/react'

// types
import { BraveWallet } from '../../constants/types'

// hooks
import { useGetOnRampAssetsQuery } from '../slices/api.slice'

export const useIsBuySupported = (
  token?: Pick<BraveWallet.BlockchainToken, 'symbol'>
) => {
  // queries
  const { data: options = undefined } = useGetOnRampAssetsQuery(
    token ? undefined : skipToken
  )

  // computed
  const isBuySupported =
    token &&
    options?.allAssetOptions.some(
      (asset) => asset.symbol.toLowerCase() === token.symbol.toLowerCase()
    )

  // render
  return isBuySupported
}
