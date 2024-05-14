// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import ProgressRing from '@brave/leo/react/progressRing'

// utils
import { getLocale } from '../../../../../../common/locale'
import {
  useGetPinnableVisibleNftIdsQuery,
  useGetUserTokensRegistryQuery
} from '../../../../../common/slices/api.slice'
import {
  selectAllVisibleUserNFTsFromQueryResult //
} from '../../../../../common/slices/entities/blockchain-token.entity'

// components
import { DecoratedNftIcon } from '../../../../shared/nft-icon/decorated-nft-icon'
import { NftCountHeading } from '../../inspects-nfts.styles'

// styles
import {
  NftListWrapper,
  List,
  NftItem,
  NftItemOverlay,
  PiningMessage
} from './nft-list.styles'
import { getAssetIdKey } from '../../../../../utils/asset-utils'

export const NftList = () => {
  // queries
  const { data: pinnableNftIds = [], isLoading: isCheckingPinnableIds } =
    useGetPinnableVisibleNftIdsQuery()
  const { nonFungibleTokens } = useGetUserTokensRegistryQuery(undefined, {
    selectFromResult: (res) => {
      return {
        nonFungibleTokens: selectAllVisibleUserNFTsFromQueryResult(res)
      }
    }
  })

  // computed
  const pinnableNftsCount = pinnableNftIds.length
  const heading = getLocale(
    pinnableNftsCount === 1
      ? 'braveWalletNftPinningInspectHeading'
      : 'braveWalletNftPinningInspectHeadingPlural'
  ).replace('$1', `${pinnableNftsCount}`)

  // render
  return (
    <NftListWrapper>
      <NftCountHeading>{heading}</NftCountHeading>
      <List>
        {nonFungibleTokens.map((token) => {
          const canBePinned = pinnableNftIds.includes(getAssetIdKey(token))
          return (
            <NftItem key={`nft-item-${token.contractAddress}-${token.tokenId}`}>
              {isCheckingPinnableIds ? (
                <NftItemOverlay>
                  <ProgressRing />
                </NftItemOverlay>
              ) : canBePinned ? null : (
                <NftItemOverlay>
                  <PiningMessage>
                    {getLocale('braveWalletNftPinningUnableToPin')}
                  </PiningMessage>
                </NftItemOverlay>
              )}
              <DecoratedNftIcon
                chainId={token.chainId}
                coinType={token.coin}
                icon={token.logo}
                responsive={true}
                disabled={!canBePinned}
              />
            </NftItem>
          )
        })}
      </List>
    </NftListWrapper>
  )
}
