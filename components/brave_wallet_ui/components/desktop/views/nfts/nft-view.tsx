// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { useGetVisibleNetworksQuery } from '../../../../common/slices/api.slice'

// types
import { BraveWallet, WalletAccountType } from '../../../../constants/types'

// hooks

// components
import { Nfts } from './components/nfts'

interface Props {
  nftsList: BraveWallet.BlockchainToken[]
  accounts: WalletAccountType[]
  onToggleShowIpfsBanner: () => void
  onShowPortfolioSettings?: () => void
}

export const NftView = ({ nftsList, accounts, onToggleShowIpfsBanner, onShowPortfolioSettings }: Props) => {
  // queries
  const { data: networks = [] } = useGetVisibleNetworksQuery()

  return (
    <Nfts
      networks={networks}
      nftList={nftsList}
      accounts={accounts}
      onToggleShowIpfsBanner={onToggleShowIpfsBanner}
      onShowPortfolioSettings={onShowPortfolioSettings}
    />
  )
}
