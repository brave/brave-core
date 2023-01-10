// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet, UserAssetInfoType } from '../../../../../../constants/types'

// Utils
import Amount from '../../../../../../utils/amount'
import { NftIcon } from '../../../../../shared/nft-icon/nft-icon'

// hooks
import { useNftPin } from '../../../../../../common/hooks/nft-pin'

// components
import { NftPinningStatusAnimation } from '../../../../nft-pinning-status-animation/nft-pinning-status-animation'

// Styled Components
import {
  NFTButton,
  NFTText,
  IconWrapper,
  DIVForClickableArea,
  NFTSymbol,
  PinnedIcon
} from './style'
import { Row } from '../../../../../shared/style'

interface Props {
  token: UserAssetInfoType
  onSelectAsset: () => void
}

export const NFTGridViewItem = (props: Props) => {
  const { token, onSelectAsset } = props

  const { getNftPinningStatus } = useNftPin()

  const pinningStatus = React.useMemo(() => {
    return getNftPinningStatus(token.asset)
  }, [token])

  return (
    <NFTButton
      onClick={onSelectAsset}
    >
      <IconWrapper>
        <DIVForClickableArea />
        <NftIcon icon={token.asset.logo} responsive={true} />
        {pinningStatus?.code === BraveWallet.TokenPinStatusCode.STATUS_PINNED && <PinnedIcon />}
      </IconWrapper>
      <Row alignItems='center' justifyContent='space-between' gap='14px' margin='6px 0 0 0'>
        <NFTText>{token.asset.name} {token.asset.tokenId ? '#' + new Amount(token.asset.tokenId).toNumber() : ''}</NFTText>
          {pinningStatus?.code !== undefined &&
            <NftPinningStatusAnimation
              size='25px'
              displayMode='nft'
              status={pinningStatus?.code}
            />
          }
      </Row>
      <NFTSymbol>{token.asset.symbol}</NFTSymbol>
    </NFTButton>
  )
}
