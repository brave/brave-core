// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../../../../constants/types'

// Utils
import Amount from '../../../../../../utils/amount'

// selectors
import { useUnsafeWalletSelector } from '../../../../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../../../../common/selectors'

// components
import { NftIconWithNetworkIcon } from '../../../../../shared/nft-icon/nft-icon-with-network-icon'

// Styled Components
import {
  NFTButton,
  NFTText,
  IconWrapper,
  DIVForClickableArea,
  NFTSymbol
} from './style'
import { Row } from '../../../../../shared/style'
import { getTokensNetwork } from '../../../../../../utils/network-utils'

interface Props {
  token: BraveWallet.BlockchainToken
  onSelectAsset: () => void
}

export const NFTGridViewItem = (props: Props) => {
  const { token, onSelectAsset } = props

  const networkList = useUnsafeWalletSelector(WalletSelectors.networkList)

  return (
    <NFTButton
      onClick={onSelectAsset}
    >
      <IconWrapper>
        <DIVForClickableArea />
        <NftIconWithNetworkIcon
          icon={token.logo}
          responsive={true}
          tokensNetwork={getTokensNetwork(networkList, token)}
        />
      </IconWrapper>
      <Row alignItems='center' justifyContent='space-between' gap='14px' margin='6px 0 0 0'>
        <NFTText>{token.name} {token.tokenId ? '#' + new Amount(token.tokenId).toNumber() : ''}</NFTText>
      </Row>
      <NFTSymbol>{token.symbol}</NFTSymbol>
    </NFTButton>
  )
}
