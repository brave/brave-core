// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../../../../constants/types'

// Utils
import { stripERC20TokenImageURL } from '../../../../../../utils/string-utils'
import Amount from '../../../../../../utils/amount'

// components
import { NftIconWithNetworkIcon } from '../../../../../shared/nft-icon/nft-icon-with-network-icon'
import { NftMorePopup } from '../nft-more-popup/nft-more-popup'
import { AddOrEditNftModal } from '../../../../popup-modals/add-edit-nft-modal/add-edit-nft-modal'

// Styled Components
import {
  NFTWrapper,
  NFTText,
  IconWrapper,
  VerticalMenuIcon,
  VerticalMenu,
  DIVForClickableArea,
  NFTSymbol
} from './style'
import { translateToNftGateway } from '../../../../../../common/async/lib'


interface Props {
  token: BraveWallet.BlockchainToken
  onSelectAsset: () => void
}

export const NFTGridViewItem = (props: Props) => {
  const { token, onSelectAsset } = props

  // state
  const [showMore, setShowMore] = React.useState<boolean>(false)
  const [showEditModal, setShowEditModal] = React.useState<boolean>(false)

  // methods
  const onToggleShowMore = React.useCallback((event: React.MouseEvent<HTMLButtonElement>) => {
    event?.stopPropagation()
    setShowMore((currentValue) => !currentValue)
  }, [])

  const onHideModal = React.useCallback(() => {
    setShowEditModal(false)
  }, [])

  const onEditNft = React.useCallback(() => {
    setShowEditModal(true)
    setShowMore(false)
  }, [])

  const [remoteImage, setRemoteImage] = React.useState<string>()
  React.useEffect(() => {
    let ignore = false
    const tokenImageURL = stripERC20TokenImageURL(token.logo)
    translateToNftGateway(tokenImageURL).then(
      (v) => {if (!ignore) setRemoteImage(v)})
    return () => {
      ignore = true
    }
  }, [token.logo])

  return (
    <>
      <NFTWrapper>
        <VerticalMenu onClick={onToggleShowMore}>
          <VerticalMenuIcon />
        </VerticalMenu>
        {showMore &&
          <NftMorePopup
            onEditNft={onEditNft}
          />
        }
        <DIVForClickableArea onClick={onSelectAsset}/>
        <IconWrapper>
          <NftIconWithNetworkIcon
            icon={remoteImage}
            responsive={true}
            chainId={token?.chainId}
            coinType={token?.coin}
          />
        </IconWrapper>
        <NFTText>{token.name} {token.tokenId ? '#' + new Amount(token.tokenId).toNumber() : ''}</NFTText>
        {token.symbol !== '' && <NFTSymbol>{token.symbol}</NFTSymbol>}
      </NFTWrapper>
      {showEditModal &&
        <AddOrEditNftModal
          nftToken={token}
          onHideForm={onHideModal}
          onClose={onHideModal}
        />
      }
    </>
  )
}
