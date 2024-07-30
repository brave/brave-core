// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'

// Types
import { BraveWallet } from '../../../../../../constants/types'
import {
  LOCAL_STORAGE_KEYS //
} from '../../../../../../common/constants/local-storage-keys'

// hooks
import {
  useRemoveUserTokenMutation,
  useUpdateNftSpamStatusMutation,
  useUpdateUserAssetVisibleMutation
} from '../../../../../../common/slices/api.slice'
import {
  useSyncedLocalStorage //
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
  NFTWrapper,
  NFTText,
  IconWrapper,
  MoreIcon,
  DIVForClickableArea,
  NFTSymbol,
  MoreButton,
  JunkMarker,
  JunkIcon,
  WatchOnlyMarker
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
  isWatchOnly
}: Props) => {
  const tokenImageURL = stripERC20TokenImageURL(token.logo)
  const [showRemoveNftModal, setShowRemoveNftModal] =
    React.useState<boolean>(false)

  // redux
  const [showNetworkLogoOnNfts] = useSyncedLocalStorage<boolean>(
    LOCAL_STORAGE_KEYS.SHOW_NETWORK_LOGO_ON_NFTS,
    false
  )

  // state
  const [showMore, setShowMore] = React.useState<boolean>(false)
  const [showEditModal, setShowEditModal] = React.useState<boolean>(false)

  // hooks
  const dispatch = useDispatch()

  // mutations
  const [updateNftSpamStatus] = useUpdateNftSpamStatusMutation()
  const [removeUserToken] = useRemoveUserTokenMutation()
  const [updateUserAssetVisible] = useUpdateUserAssetVisibleMutation()

  // methods
  const onToggleShowMore = React.useCallback(
    (event: React.MouseEvent<HTMLButtonElement>) => {
      event?.stopPropagation()
      setShowMore((currentValue) => !currentValue)
    },
    []
  )

  const onHideModal = React.useCallback(() => {
    setShowEditModal(false)
  }, [])

  const onEditNft = React.useCallback(() => {
    setShowEditModal(true)
    setShowMore(false)
  }, [])

  const onHideNft = React.useCallback(async () => {
    setShowMore(false)
    await updateUserAssetVisible({
      token,
      isVisible: false
    }).unwrap()
  }, [token, updateUserAssetVisible])

  const onUnHideNft = React.useCallback(async () => {
    setShowMore(false)
    await updateUserAssetVisible({
      token,
      isVisible: true
    }).unwrap()
    if (isTokenSpam) {
      // remove from spam
      await updateNftSpamStatus({ token, isSpam: false })
    }
  }, [updateUserAssetVisible, token, isTokenSpam, updateNftSpamStatus])

  const onUnSpam = async () => {
    setShowMore(false)
    await updateNftSpamStatus({ token, isSpam: false })
    dispatch(WalletActions.refreshNetworksAndTokens())
  }

  const onMarkAsSpam = async () => {
    setShowMore(false)
    await updateNftSpamStatus({ token, isSpam: true })
    dispatch(WalletActions.refreshNetworksAndTokens())
  }

  const onConfirmDelete = async () => {
    setShowRemoveNftModal(false)
    await removeUserToken(getAssetIdKey(token)).unwrap()
  }

  return (
    <>
      <NFTWrapper>
        <NftMorePopup
          isOpen={showMore}
          isTokenHidden={isTokenHidden}
          isTokenSpam={isTokenSpam}
          onEditNft={onEditNft}
          onHideNft={onHideNft}
          onUnHideNft={onUnHideNft}
          onUnSpam={onUnSpam}
          onMarkAsSpam={onMarkAsSpam}
          onRemoveNft={() => {
            setShowMore(false)
            setShowRemoveNftModal(true)
          }}
          onClose={() => setShowMore(false)}
        />
        <DIVForClickableArea onClick={() => onSelectAsset(token)} />
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
          <NFTText>{token.name}</NFTText>
          <MoreButton onClick={onToggleShowMore}>
            <MoreIcon />
          </MoreButton>
        </Row>
        {token.symbol !== '' && <NFTSymbol>{token.symbol}</NFTSymbol>}
      </NFTWrapper>
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
