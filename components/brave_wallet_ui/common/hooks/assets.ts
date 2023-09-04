// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { WalletSelectors } from '../selectors'

// hooks
import { useUnsafeWalletSelector } from './use-safe-selector'
import {
  useGetSelectedChainQuery
} from '../slices/api.slice'

export function useAssets () {
  // redux
  const userVisibleTokensInfo = useUnsafeWalletSelector(
    WalletSelectors.userVisibleTokensInfo
  )

  // queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()

  // memos
  const assetsByNetwork = React.useMemo(() => {
    if (!userVisibleTokensInfo || !selectedNetwork) {
      return []
    }
    // We also filter by coinType here because localhost
    // networks share the same chainId.
    return userVisibleTokensInfo.filter((token) =>
      token.chainId === selectedNetwork.chainId &&
      token.coin === selectedNetwork.coin
    )
  }, [userVisibleTokensInfo, selectedNetwork])

  return assetsByNetwork
}

export default useAssets
