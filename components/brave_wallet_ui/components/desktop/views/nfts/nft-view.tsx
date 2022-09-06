// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'

// types
import {
  SupportedTestNetworks,
  WalletState
} from '../../../../constants/types'

// components
import { Nfts } from './components/nfts'
import { AllNetworksOption } from '../../../../options/network-filter-options'

export const NftView = () => {
  // redux
  const networkList = useSelector(({ wallet }: { wallet: WalletState }) => wallet.networkList)
  const userVisibleTokensInfo = useSelector(({ wallet }: { wallet: WalletState}) => wallet.userVisibleTokensInfo)
  const selectedNetworkFilter = useSelector(({ wallet }: { wallet: WalletState }) => wallet.selectedNetworkFilter)

  const fungibleTokens = React.useMemo(() => {
    if (selectedNetworkFilter.chainId === AllNetworksOption.chainId) {
      return userVisibleTokensInfo.filter((token) => !SupportedTestNetworks.includes(token.chainId) && token.isErc721)
    }

    return userVisibleTokensInfo.filter(token => token.chainId === selectedNetworkFilter.chainId && token.isErc721)
  }, [userVisibleTokensInfo, selectedNetworkFilter])

  return (
    <Nfts
      networks={networkList}
      nftList={fungibleTokens}
    />
  )
}
