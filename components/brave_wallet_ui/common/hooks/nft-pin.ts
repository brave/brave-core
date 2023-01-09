// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// constants
import { BraveWallet } from '../../constants/types'

// selectors
import { PageSelectors } from '../../page/selectors'
import { WalletSelectors } from '../selectors'
import { useUnsafePageSelector, useUnsafeWalletSelector } from './use-safe-selector'

// utils
import { reverseHttpifiedIpfsUrl, stripERC20TokenImageURL } from '../../utils/string-utils'
import { LOCAL_STORAGE_KEYS } from '../constants/local-storage-keys'
import { getAssetIdKey } from '../../utils/asset-utils'

export function useNftPin () {
  const [isIpfsBannerVisible, setIsIpfsBannerVisible] = React.useState<boolean>(
    localStorage.getItem(LOCAL_STORAGE_KEYS.IS_IPFS_BANNER_HIDDEN) === 'false' ||
    localStorage.getItem(LOCAL_STORAGE_KEYS.IS_IPFS_BANNER_HIDDEN) === null
  )

  const userVisibleTokensInfo = useUnsafeWalletSelector(
    WalletSelectors.userVisibleTokensInfo
  )

  // redux
  const nftsPinningStatus = useUnsafePageSelector(PageSelectors.nftsPinningStatus)

  const pinnedNftsCount = React.useMemo(() => {
    return Object.keys(nftsPinningStatus).reduce((accumulator, currentValue) => {
      const status = nftsPinningStatus[currentValue]
      if (status?.code === BraveWallet.TokenPinStatusCode.STATUS_PINNED) {
        return accumulator += 1
      }

      return accumulator
    }, 0)
  }, [nftsPinningStatus])

  const [nonFungibleTokens, pinnableNfts] = React.useMemo(() => {
    const tokens = userVisibleTokensInfo.filter(
      (token) => token.isErc721 || token.isNft
    )
    const pinnable: BraveWallet.BlockchainToken[] = []
    const nfts = tokens.map(token => {
      const canBePinned = reverseHttpifiedIpfsUrl(stripERC20TokenImageURL(token.logo)).startsWith('ipfs://')
      if (canBePinned) {
        pinnable.push(token)
      }
      return { canBePinned, token }
    })
    return [nfts, pinnable]
  }, [userVisibleTokensInfo])

  const pinningStatusSummary: BraveWallet.TokenPinStatusCode = React.useMemo(() => {
    if (pinnableNfts.length === pinnedNftsCount) {
      return BraveWallet.TokenPinStatusCode.STATUS_PINNED
    }

    const isUploading = Object.values(nftsPinningStatus).some(status => status?.code === BraveWallet.TokenPinStatusCode.STATUS_PINNING_IN_PROGRESS)
    if (isUploading) return BraveWallet.TokenPinStatusCode.STATUS_PINNING_IN_PROGRESS

    return BraveWallet.TokenPinStatusCode.STATUS_NOT_PINNED
  }, [nftsPinningStatus, pinnableNfts, pinnedNftsCount])

  const onToggleShowIpfsBanner = React.useCallback(() => {
    setIsIpfsBannerVisible(prev => {
      window.localStorage.setItem(
        LOCAL_STORAGE_KEYS.IS_IPFS_BANNER_HIDDEN,
        prev ? 'false' : 'true'
      )
      return !prev
    })
  }, [])

  const getNftPinningStatus = React.useCallback((token: BraveWallet.BlockchainToken): BraveWallet.TokenPinStatus | undefined => {
    return nftsPinningStatus[getAssetIdKey(token)]
  }, [nftsPinningStatus])

  return {
    nonFungibleTokens,
    pinnableNftsCount: pinnableNfts.length,
    pinnedNftsCount,
    pinnableNfts,
    isIpfsBannerVisible,
    pinningStatusSummary,
    onToggleShowIpfsBanner,
    getNftPinningStatus
  }
}
