// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// utils
import { useNftPin } from '../../../../../common/hooks/nft-pin'
import { getLocale } from '../../../../../../common/locale'

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

export const NftList = () => {
  // hooks
  const { pinnableNftsCount, nonFungibleTokens } = useNftPin()

  const heading = React.useMemo(() => {
    return getLocale(
      pinnableNftsCount === 1
        ? 'braveWalletNftPinningInspectHeading'
        : 'braveWalletNftPinningInspectHeadingPlural'
    ).replace('$1', `${pinnableNftsCount}`)
  }, [pinnableNftsCount])

  return (
    <NftListWrapper>
      <NftCountHeading>{heading}</NftCountHeading>
      <List>
        {nonFungibleTokens.map(({ canBePinned, token }) => (
          <NftItem key={`nft-item-${token.contractAddress}-${token.tokenId}`}>
            {!canBePinned && (
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
        ))}
      </List>
    </NftListWrapper>
  )
}
