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
  useGetIpfsGatewayTranslatedNftUrlQuery,
  useRestoreHiddenTokenMutation,
  useRemoveUserTokenMutation,
  useUpdateNftSpamStatusMutation,
  useHideOrDeleteTokenMutation
} from '../../../../../../common/slices/api.slice'

// actions
import { WalletActions } from '../../../../../../common/actions'

// selectors
import { useSafeWalletSelector } from '../../../../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../../../../common/selectors'

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
  JunkIcon
} from './style'
import { Row } from '../../../../../shared/style'

interface Props {
  token: BraveWallet.BlockchainToken
  isTokenHidden: boolean
  isTokenSpam: boolean
  onSelectAsset: () => void
}

export const NFTGridViewItem = (props: Props) => {
  const { token, isTokenHidden, isTokenSpam, onSelectAsset } = props
  const tokenImageURL = stripERC20TokenImageURL(token.logo)
  const [showRemoveNftModal, setShowRemoveNftModal] =
    React.useState<boolean>(false)

  // redux
  const showNetworkLogoOnNfts = useSafeWalletSelector(
    WalletSelectors.showNetworkLogoOnNfts
  )

  // state
  const [showMore, setShowMore] = React.useState<boolean>(false)
  const [showEditModal, setShowEditModal] = React.useState<boolean>(false)

  // queries
  const { data: remoteImage } = useGetIpfsGatewayTranslatedNftUrlQuery(
    tokenImageURL || skipToken
  )

  // hooks
  const dispatch = useDispatch()
  const { addNftToDeletedNftsList } = useAssetManagement()

  // mutations
  const [updateNftSpamStatus] = useUpdateNftSpamStatusMutation()
  const [removeUserToken] = useRemoveUserTokenMutation()
  const [hideOrDeleteToken] = useHideOrDeleteTokenMutation()
  const [restoreHiddenToken] = useRestoreHiddenTokenMutation()

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
    await hideOrDeleteToken({
      mode: 'hide',
      tokenId: getAssetIdKey(token)
    })
    dispatch(
      WalletActions.refreshNetworksAndTokens({ skipBalancesRefresh: true })
    )
  }, [token, hideOrDeleteToken])

  const onUnHideNft = React.useCallback(async () => {
    setShowMore(false)
    await restoreHiddenToken(getAssetIdKey(token))
    if (isTokenSpam) {
      // remove from spam
      await updateNftSpamStatus({ token, status: false })
    }
    dispatch(
      WalletActions.refreshNetworksAndTokens({ skipBalancesRefresh: true })
    )
  }, [token, restoreHiddenToken, isTokenSpam])

  const onUnSpam = async () => {
    setShowMore(false)
    await updateNftSpamStatus({ token, status: false })
    dispatch(
      WalletActions.refreshNetworksAndTokens({ skipBalancesRefresh: true })
    )
  }

  const onConfirmDelete = async () => {
    setShowRemoveNftModal(false)

    await removeUserToken(token)
    addNftToDeletedNftsList(token)
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
          onRemoveNft={() => {
            setShowMore(false)
            setShowRemoveNftModal(true)
          }}
          onClose={() => setShowMore(false)}
        />
        <DIVForClickableArea onClick={onSelectAsset} />
        {isTokenSpam && (
          <JunkMarker>
            {getLocale('braveWalletNftJunk')}
            <JunkIcon />
          </JunkMarker>
        )}
        <IconWrapper>
          <DecoratedNftIcon
            icon={remoteImage}
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
