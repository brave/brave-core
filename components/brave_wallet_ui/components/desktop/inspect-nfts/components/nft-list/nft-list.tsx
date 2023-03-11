// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// utils
import { useUnsafeWalletSelector } from '../../../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../../../common/selectors'
import { getTokensNetwork } from '../../../../../utils/network-utils'
import { useNftPin } from '../../../../../common/hooks/nft-pin'
import { getLocale } from '../../../../../../common/locale'

// components
import { NftIconWithNetworkIcon } from '../../../../shared/nft-icon/nft-icon-with-network-icon'
import { NftCountHeading } from '../../inspects-nfts.styles'

// styles
import { NftListWrapper, List, NftItem, NftItemOverlay, PiningMessage } from './nft-list.styles'

export const NftList = () => {
  const networkList = useUnsafeWalletSelector(WalletSelectors.networkList)

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
              tokensNetwork={getTokensNetwork(networkList, token)}
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
