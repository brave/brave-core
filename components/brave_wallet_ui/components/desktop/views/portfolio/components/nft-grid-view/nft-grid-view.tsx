// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { UserAssetInfoType, BraveWallet } from 'components/brave_wallet_ui/constants/types'

// Components
import { NFTGridViewItem } from './nft-grid-view-item'

// Styled Components
import {
  StyledWrapper
} from './style'

interface Props {
  nonFungibleTokens: UserAssetInfoType[]
  onSelectAsset: (asset: BraveWallet.BlockchainToken | undefined) => void
}

export const NFTGridView = (props: Props) => {
  const { nonFungibleTokens, onSelectAsset } = props

  return (
    <StyledWrapper>
      {nonFungibleTokens.map((token: UserAssetInfoType) =>
        <NFTGridViewItem
          key={`${token.asset.tokenId}-${token.asset.contractAddress}`}
          token={token}
          onSelectAsset={() => onSelectAsset(token.asset)}
        />
      )}
    </StyledWrapper>
  )
}
