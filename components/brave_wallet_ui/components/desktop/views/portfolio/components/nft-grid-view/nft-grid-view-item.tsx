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
  NFTWrapper,
  NFTText,
  IconWrapper,
  VerticalMenuIcon,
  VerticalMenu,
  DIVForClickableArea
} from './style'
import { NftMorePopup } from '../nft-more-popup/nft-more-popup'
import { AddOrEditNftModal } from '../../../../popup-modals/add-edit-nft-modal/add-edit-nft-modal'

interface Props {
  token: BraveWallet.BlockchainToken
  onSelectAsset: () => void
}

export const NFTGridViewItem = (props: Props) => {
  const { token, onSelectAsset } = props
  const { asset } = token

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

  // memos
  const remoteImage = React.useMemo(() => {
    const tokenImageURL = stripERC20TokenImageURL(token.asset.logo)
    return httpifyIpfsUrl(tokenImageURL)
  }, [token.asset.logo])

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
          <NftIcon icon={remoteImage} responsive={true} />
        </IconWrapper>
        <NFTText>{asset.name} {asset.tokenId ? '#' + new Amount(asset.tokenId).toNumber() : ''}</NFTText>
      </NFTWrapper>
      {showEditModal &&
        <AddOrEditNftModal
          nftToken={asset}
          onHideForm={onHideModal}
          onClose={onHideModal}
        />
      }
    </>
  )
}
