// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { UserAssetInfoType } from 'components/brave_wallet_ui/constants/types'

// Utils
import { stripERC20TokenImageURL, httpifyIpfsUrl } from '../../../../../../utils/string-utils'
import Amount from '../../../../../../utils/amount'
import { NftIcon } from '../../../../../shared/nft-icon/nft-icon'

// Styled Components
import {
  NFTButton,
  NFTText,
  IconWrapper,
  DIVForClickableArea
} from './style'

interface Props {
  token: UserAssetInfoType
  onSelectAsset: () => void
}

export const NFTGridViewItem = (props: Props) => {
  const { token, onSelectAsset } = props
  const tokenImageURL = stripERC20TokenImageURL(token.asset.logo)

  const remoteImage = React.useMemo(() => {
    return httpifyIpfsUrl(tokenImageURL)
  }, [tokenImageURL])

  return (
    <NFTButton
      onClick={onSelectAsset}
    >
      <IconWrapper>
        <DIVForClickableArea />
        <NftIcon icon={remoteImage} responsive={true} />
      </IconWrapper>
      <NFTText>{token.asset.name} {'#' + new Amount(token.asset.tokenId).toNumber()}</NFTText>
    </NFTButton>
  )
}
