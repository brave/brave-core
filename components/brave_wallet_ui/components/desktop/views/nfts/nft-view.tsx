// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { useSelector } from 'react-redux'
import { useGetVisibleNetworksQuery } from '../../../../common/slices/api.slice'

// types
import {
  SupportedTestNetworks,
  WalletState
} from '../../../../constants/types'
import { AllNetworksOption } from '../../../../options/network-filter-options'

// hooks

// components
import { Nfts } from './components/nfts'

interface Props {
  onToggleShowIpfsBanner: () => void
}

export const NftView = ({ onToggleShowIpfsBanner }: Props) => {
  // redux
  const userVisibleTokensInfo = useSelector(({ wallet }: { wallet: WalletState }) => wallet.userVisibleTokensInfo)
  const selectedNetworkFilter = useSelector(({ wallet }: { wallet: WalletState }) => wallet.selectedNetworkFilter)

  // queries
  const { data: networks = [] } = useGetVisibleNetworksQuery()

  // memos
  const nonFungibleTokens = React.useMemo(() => {
    if (selectedNetworkFilter.chainId === AllNetworksOption.chainId) {
      return userVisibleTokensInfo.filter(
        (token) =>
          !SupportedTestNetworks.includes(token.chainId) &&
          (token.isErc721 || token.isNft)
      )
    }

    return userVisibleTokensInfo.filter(
      (token) =>
        token.chainId === selectedNetworkFilter.chainId &&
        (token.isErc721 || token.isNft)
    )
  }, [userVisibleTokensInfo, selectedNetworkFilter.chainId])

  return (
    <Nfts
      networks={networks}
      nftList={nonFungibleTokens}
      onToggleShowIpfsBanner={onToggleShowIpfsBanner}
    />
  )
}
