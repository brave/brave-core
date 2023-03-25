// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Hooks
import { useHasAccount } from './'

// Constants
import { BraveWallet } from '../../constants/types'
import { useGetSelectedChainQuery } from '../slices/api.slice'

export function usePrevNetwork () {
  // queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()

  // hooks
  const { hasFilAccount, hasSolAccount } = useHasAccount()

  // state
  const [prevNetwork, setPrevNetwork] = React.useState<BraveWallet.NetworkInfo>()

  React.useEffect(() => {
    if ((selectedNetwork?.coin === BraveWallet.CoinType.SOL && hasSolAccount) ||
      (selectedNetwork?.coin === BraveWallet.CoinType.FIL && hasFilAccount) ||
      selectedNetwork?.coin === BraveWallet.CoinType.ETH) {
      setPrevNetwork(selectedNetwork)
    }
  }, [selectedNetwork, hasSolAccount, hasFilAccount])
  return {
    prevNetwork
  }
}

export default usePrevNetwork
