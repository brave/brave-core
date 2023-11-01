// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { useGetVisibleNetworksQuery } from '../../../../common/slices/api.slice'
import {
  TokenBalancesRegistry //
} from '../../../../common/slices/entities/token-balance.entity'

// types
import { BraveWallet } from '../../../../constants/types'

// hooks

// components
import { Nfts } from './components/nfts'

interface Props {
  nftsList: BraveWallet.BlockchainToken[]
  accounts: BraveWallet.AccountInfo[]
  onShowPortfolioSettings?: () => void
  tokenBalancesRegistry: TokenBalancesRegistry | undefined
}

export const NftView = ({
  nftsList,
  accounts,
  onShowPortfolioSettings,
  tokenBalancesRegistry
}: Props) => {
  // queries
  const { data: networks = [] } = useGetVisibleNetworksQuery()

  return (
    <Nfts
      networks={networks}
      nftList={nftsList}
      accounts={accounts}
      onShowPortfolioSettings={onShowPortfolioSettings}
      tokenBalancesRegistry={tokenBalancesRegistry}
    />
  )
}
