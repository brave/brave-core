// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// redux
import { useAppDispatch } from '../../../../../../common/hooks/use-redux'

// Types
import { BraveWallet } from '../../../../../../constants/types'
import {
  LOCAL_STORAGE_KEYS, //
} from '../../../../../../common/constants/local-storage-keys'

// hooks
import {
  useRemoveUserTokenMutation,
  useUpdateNftSpamStatusMutation,
  useUpdateUserAssetVisibleMutation,
} from '../../../../../../common/slices/api.slice'
import {
  useSyncedLocalStorage, //
} from '../../../../../../common/hooks/use_local_storage'

// actions
import { WalletActions } from '../../../../../../common/actions'

// Utils
import { stripERC20TokenImageURL } from '../../../../../../utils/string-utils'
import { getLocale } from '../../../../../../../common/locale'
import { getAssetIdKey } from '../../../../../../utils/asset-utils'

// components
import { DecoratedNftIcon } from '../../../../../shared/nft-icon/decorated-nft-icon'
import { NftMorePopup } from '../nft-more-popup/nft-more-popup'
import { AddOrEditNftModal } from '../../../../popup-modals/add-edit-nft-modal/add-edit-nft-modal'
import { RemoveNftModal } from '../../../../popup-modals/remove-nft-modal/remove-nft-modal'

// Styled Components
import {
  GridItemWrapper,
  NFTText,
  IconWrapper,
  MoreIcon,
  DIVForClickableArea,
  NFTSymbol,
  MoreButton,
  JunkMarker,
  JunkIcon,
  WatchOnlyMarker,
} from './style'
import { Row } from '../../../../../shared/style'

interface Props {
  token: BraveWallet.BlockchainToken
  isTokenHidden: boolean
  isTokenSpam: boolean
  onSelectAsset: (token: BraveWallet.BlockchainToken) => void
  isWatchOnly?: boolean
}

export const NFTGridViewItem = ({
  token,
  isTokenHidden,
  isTokenSpam,
  onSelectAsset,
  isWatchOnly,
}: Props) => {
  const tokenImageURL = stripERC20TokenImageURL(token.logo)
  const [showRemoveNftModal, setShowRemoveNftModal] =
    React.useState<boolean>(false)

  // redux
  const [showNetworkLogoOnNfts] = useSyncedLocalStorage<boolean>(
    LOCAL_STORAGE_KEYS.SHOW_NETWORK_LOGO_ON_NFTS,
    false,
  )

  // state
  const [showEditModal, setShowEditModal] = React.useState<boolean>(false)

  // hooks
  const dispatch = useAppDispatch()

  // mutations
  const [updateNftSpamStatus] = useUpdateNftSpamStatusMutation()
  const [removeUserToken] = useRemoveUserTokenMutation()
  const [updateUserAssetVisible] = useUpdateUserAssetVisibleMutation()

  // methods
  const onHideModal = React.useCallback(() => {
    setShowEditModal(false)
  }, [])

  const onEditNft = React.useCallback(() => {
    setShowEditModal(true)
  }, [])

  const onHideNft = React.useCallback(async () => {
    await updateUserAssetVisible({
      token,
      isVisible: false,
    }).unwrap()
  }, [token, updateUserAssetVisible])

  const onUnHideNft = React.useCallback(async () => {
    await updateUserAssetVisible({
      token,
      isVisible: true,
    }).unwrap()
    if (isTokenSpam) {
      // remove from spam
      await updateNftSpamStatus({ token, isSpam: false })
    }
  }, [updateUserAssetVisible, token, isTokenSpam, updateNftSpamStatus])

  const onUnSpam = async () => {
    await updateNftSpamStatus({ token, isSpam: false })
    dispatch(WalletActions.refreshNetworksAndTokens())
  }

  const onMarkAsSpam = async () => {
    await updateNftSpamStatus({ token, isSpam: true })
    dispatch(WalletActions.refreshNetworksAndTokens())
  }

  const onConfirmDelete = async () => {
    setShowRemoveNftModal(false)
    await removeUserToken(getAssetIdKey(token)).unwrap()
  }

  return (
    <>
      <GridItemWrapper>
        {isTokenSpam && (
          <JunkMarker>
            {getLocale('braveWalletNftJunk')}
            <JunkIcon />
          </JunkMarker>
        )}
        {isWatchOnly && (
          <WatchOnlyMarker>
            {
              getLocale('braveWalletWatchOnly') //
            }
          </WatchOnlyMarker>
        )}
        <IconWrapper>
          <DIVForClickableArea onClick={() => onSelectAsset(token)} />
          <DecoratedNftIcon
            icon={tokenImageURL}
            responsive={true}
            chainId={token?.chainId}
            coinType={token?.coin}
            hideNetworkIcon={!showNetworkLogoOnNfts}
          />
        </IconWrapper>

        <Row
          justifyContent='space-between'
          margin='8px 0 0 0'
        >
          <NFTText
            textColor='primary'
            variant='default.semibold'
            onClick={() => onSelectAsset(token)}
          >
            {token.name}
          </NFTText>
          <NftMorePopup
            isTokenHidden={isTokenHidden}
            isTokenSpam={isTokenSpam}
            onEditNft={onEditNft}
            onHideNft={onHideNft}
            onUnHideNft={onUnHideNft}
            onUnSpam={onUnSpam}
            onMarkAsSpam={onMarkAsSpam}
            onRemoveNft={() => setShowRemoveNftModal(true)}
          >
            <MoreButton slot='anchor-content'>
              <MoreIcon />
            </MoreButton>
          </NftMorePopup>
        </Row>
        {token.symbol !== '' && (
          <NFTSymbol
            textColor='secondary'
            variant='small.regular'
            onClick={() => onSelectAsset(token)}
          >
            {token.symbol}
          </NFTSymbol>
        )}
      </GridItemWrapper>
      {showEditModal && (
        <AddOrEditNftModal
          nftToken={token}
          onHideForm={onHideModal}
          onClose={onHideModal}
        />
      )}

      {showRemoveNftModal && (
        <RemoveNftModal
          onConfirm={onConfirmDelete}
          onCancel={() => setShowRemoveNftModal(false)}
        />
      )}
    </>
  )
}
