// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { skipToken } from '@reduxjs/toolkit/query'

// Types
import { BraveWallet } from '../../../../../../constants/types'

// hooks
import {
  useAssetManagement //
} from '../../../../../../common/hooks/assets-management'
import {
  useGetIpfsGatewayTranslatedNftUrlQuery //
} from '../../../../../../common/slices/api.slice'

// actions
import { WalletActions } from '../../../../../../common/actions'

// selectors
import { useSafeWalletSelector } from '../../../../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../../../../common/selectors'

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

interface Props {
  token: BraveWallet.BlockchainToken
  isHidden: boolean
  onSelectAsset: () => void
}

export const NFTGridViewItem = (props: Props) => {
  const { token, isHidden, onSelectAsset } = props
  const tokenImageURL = stripERC20TokenImageURL(token.logo)

  // redux
  const showNetworkLogoOnNfts = useSafeWalletSelector(WalletSelectors.showNetworkLogoOnNfts)

  // state
  const [showMore, setShowMore] = React.useState<boolean>(false)
  const [showEditModal, setShowEditModal] = React.useState<boolean>(false)

  // queries
  const { data: remoteImage } = useGetIpfsGatewayTranslatedNftUrlQuery(
    tokenImageURL || skipToken
  )

  // hooks
  const dispatch = useDispatch()
  const { addOrRemoveTokenInLocalStorage } = useAssetManagement()

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

  const onHideNft = React.useCallback(() => {
    setShowMore(false)
    addOrRemoveTokenInLocalStorage(token, 'remove')
    dispatch(WalletActions.refreshNetworksAndTokens({}))
  }, [token, addOrRemoveTokenInLocalStorage])

  const onUnHideNft = React.useCallback(() => {
    setShowMore(false)
    addOrRemoveTokenInLocalStorage(token, 'add')
    dispatch(WalletActions.refreshNetworksAndTokens({}))
  }, [token, addOrRemoveTokenInLocalStorage])

  return (
    <>
      <NFTWrapper>
        <VerticalMenu
          onClick={onToggleShowMore}
        >
          <VerticalMenuIcon />
        </VerticalMenu>
        {showMore && (
          <NftMorePopup
            isHidden={isHidden}
            onEditNft={onEditNft}
            onHideNft={onHideNft}
            onUnHideNft={onUnHideNft}
          />
        )}
        <DIVForClickableArea onClick={onSelectAsset} />
        <IconWrapper>
          <NftIconWithNetworkIcon
            icon={remoteImage}
            responsive={true}
            chainId={token?.chainId}
            coinType={token?.coin}
            hideNetworkIcon={!showNetworkLogoOnNfts}
          />
        </IconWrapper>
        <NFTText>
          {token.name}{' '}
          {token.tokenId ? '#' + new Amount(token.tokenId).toNumber() : ''}
        </NFTText>
        {token.symbol !== '' && <NFTSymbol>{token.symbol}</NFTSymbol>}
      </NFTWrapper>
      {showEditModal && (
        <AddOrEditNftModal
          nftToken={token}
          onHideForm={onHideModal}
          onClose={onHideModal}
        />
      )}
    </>
  )
}
