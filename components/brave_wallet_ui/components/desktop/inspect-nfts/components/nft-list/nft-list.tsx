// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// utils
import { useNftPin } from '../../../../../common/hooks/nft-pin'
import { getLocale } from '../../../../../../common/locale'
import { useGetAllNetworksQuery } from '../../../../../common/slices/api.slice'
import {
  networkEntityAdapter,
  emptyNetworksRegistry
} from '../../../../../common/slices/entities/network.entity'

// components
import { NftIconWithNetworkIcon } from '../../../../shared/nft-icon/nft-icon-with-network-icon'
import { NftCountHeading } from '../../inspects-nfts.styles'

// styles
import { NftListWrapper, List, NftItem, NftItemOverlay, PiningMessage } from './nft-list.styles'

export const NftList = () => {
  // queries
  const { data: networksRegistry = emptyNetworksRegistry } =
    useGetAllNetworksQuery()

  // hooks
  const { pinnableNftsCount, nonFungibleTokens } = useNftPin()

  return (
    <NftListWrapper>
      <NftCountHeading>
        {getLocale('braveWalletNftPinningInspectHeading')
          .replace('$1', `${pinnableNftsCount}`)
          .replace('$2', `${nonFungibleTokens.length}`)}
      </NftCountHeading>
      <List>
        {nonFungibleTokens.map(({ canBePinned, token }) => (
          <NftItem key={`nft-item-${token.contractAddress}-${token.tokenId}`}>
            {!canBePinned && (
              <NftItemOverlay>
                <PiningMessage>{getLocale('braveWalletNftPinningUnableToPin')}</PiningMessage>
              </NftItemOverlay>
            )}
            <NftIconWithNetworkIcon
              tokensNetwork={
                networksRegistry.entities[networkEntityAdapter.selectId(token)]
              }
              icon={token.logo}
              responsive={true}
              disabled={!canBePinned}
            />
          </NftItem>
        ))}
      </List>
    </NftListWrapper>
  )
}
