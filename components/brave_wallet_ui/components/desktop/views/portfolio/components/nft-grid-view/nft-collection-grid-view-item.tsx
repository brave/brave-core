// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../../../../constants/types'

// Utils
import { getAssetIdKey } from '../../../../../../utils/asset-utils'

// Styled Components
import { NFTWrapper, NFTText, DIVForClickableArea } from './style'
import { Row } from '../../../../../shared/style'
import { NftIcon } from '../../../../../shared/nft-icon/nft-icon'
import {
  CollectionGrid,
  EmptyCollectionGridItem,
  NftIconStyles
} from './nft-collection-grid-view-item.styles'

interface Props {
  collectionToken: BraveWallet.BlockchainToken
  tokensInCollection: BraveWallet.BlockchainToken[]
  onSelectAsset: (token: BraveWallet.BlockchainToken) => void
}

export const NftCollectionGridViewItem = ({
  collectionToken,
  tokensInCollection,
  onSelectAsset
}: Props) => {
  // render
  if (tokensInCollection.length === 0) {
    return null
  }

  return (
    <NFTWrapper>
      <DIVForClickableArea onClick={() => onSelectAsset(collectionToken)} />
      <CollectionGrid>
        {/* display a grid of 4 token images */}
        {tokensInCollection.slice(0, 4).map((token, index) => {
          return (
            <div key={getAssetIdKey(token)}>
              <NftIcon
                icon={token?.logo}
                iconStyles={NftIconStyles}
                responsive={false}
              />
            </div>
          )
        })}
        {tokensInCollection.length < 2 && (
          <EmptyCollectionGridItem key='empty-grid-item-1' />
        )}
        {tokensInCollection.length < 3 && (
          <EmptyCollectionGridItem key='empty-grid-item-2' />
        )}
        {tokensInCollection.length < 4 && (
          <EmptyCollectionGridItem key='empty-grid-item-3' />
        )}
      </CollectionGrid>

      <Row
        justifyContent='space-between'
        margin='8px 0 0 0'
      >
        <NFTText>{collectionToken.name}</NFTText>
        <NFTText>{tokensInCollection.length}</NFTText>
      </Row>
    </NFTWrapper>
  )
}
