// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// constants
import { BraveWallet } from '../../constants/types'

// selectors
import { PageSelectors } from '../../page/selectors'
import { WalletSelectors } from '../selectors'
import { useSafePageSelector, useSafeWalletSelector, useUnsafePageSelector, useUnsafeWalletSelector } from './use-safe-selector'

// utils
import { LOCAL_STORAGE_KEYS } from '../constants/local-storage-keys'
import { getAssetIdKey } from '../../utils/asset-utils'
import { PinningStatusType } from '../../page/constants/action_types'
import { useLib } from './useLib'
import { areSupportedForPinning } from '../../common/async/lib'

export enum OverallPinningStatus {
  PINNING_FINISHED,
  PINNING_IN_PROGRESS,
  NO_PINNED_ITEMS
}

export function useNftPin () {
  const [nonFungibleTokens, setNonFungibleTokens] = React.useState<Array<{ canBePinned: boolean, token: BraveWallet.BlockchainToken }>>([])
  const [pinnableNfts, setPinnableNfts] = React.useState<BraveWallet.BlockchainToken[]>([])

  // redux
  const userVisibleTokensInfo = useUnsafeWalletSelector(
    WalletSelectors.userVisibleTokensInfo
  )

  // redux
  const isNftPinningFeatureEnabled = useSafeWalletSelector(WalletSelectors.isNftPinningFeatureEnabled)
  const nftsPinningStatus = useUnsafePageSelector(PageSelectors.nftsPinningStatus)
  const isAutoPinEnabled = useSafePageSelector(PageSelectors.isAutoPinEnabled)


  // hooks
  const { isTokenPinningSupported } = useLib()

  // memos
  const pinnedNftsCount = React.useMemo(() => {
    return Object.keys(nftsPinningStatus).reduce((accumulator, currentValue) => {
      const status = nftsPinningStatus[currentValue]
      if (status?.code === BraveWallet.TokenPinStatusCode.STATUS_PINNED) {
        return accumulator += 1
      }

      return accumulator
    }, 0)
  }, [nftsPinningStatus])

  const inProgressNftCount = React.useMemo(() => {
    return Object.keys(nftsPinningStatus).reduce((accumulator, currentValue) => {
      const status = nftsPinningStatus[currentValue]
      if (status?.code === BraveWallet.TokenPinStatusCode.STATUS_PINNING_IN_PROGRESS ||
          status?.code === BraveWallet.TokenPinStatusCode.STATUS_PINNING_PENDING) {
        return accumulator += 1
      }

      return accumulator
    }, 0)
  }, [nftsPinningStatus])

  const pinningStatusSummary: BraveWallet.TokenPinStatusCode = React.useMemo(() => {
    if (pinnedNftsCount > 0 && inProgressNftCount === 0) {
      return OverallPinningStatus.PINNING_FINISHED
    } else if (inProgressNftCount > 0) {
      return OverallPinningStatus.PINNING_IN_PROGRESS
    }

    return OverallPinningStatus.NO_PINNED_ITEMS
  }, [nftsPinningStatus, pinnableNfts.length,
      pinnedNftsCount, inProgressNftCount])

  const [isIpfsBannerEnabled, setIsIpfsBannerEnabled] = React.useState<boolean>(
    localStorage.getItem(LOCAL_STORAGE_KEYS.IS_IPFS_BANNER_HIDDEN) === 'false' ||
    localStorage.getItem(LOCAL_STORAGE_KEYS.IS_IPFS_BANNER_HIDDEN) === null
  )

  const isIpfsBannerVisible = React.useMemo(() => {
    return isIpfsBannerEnabled &&
      (pinningStatusSummary !== OverallPinningStatus.NO_PINNED_ITEMS ||
       !isAutoPinEnabled)
  }, [isIpfsBannerEnabled, pinningStatusSummary, isAutoPinEnabled])

  // methods
  const onToggleShowIpfsBanner = React.useCallback(() => {
    setIsIpfsBannerEnabled(prev => {
      window.localStorage.setItem(
        LOCAL_STORAGE_KEYS.IS_IPFS_BANNER_HIDDEN,
        !prev ? 'false' : 'true'
      )
      return !prev
    })
  }, [])

  const getNftPinningStatus = React.useCallback((token: BraveWallet.BlockchainToken): PinningStatusType | undefined => {
    return nftsPinningStatus[getAssetIdKey(token)]
  }, [nftsPinningStatus])

  // effects
  React.useEffect(() => {
    let ignore = false
    if (!isNftPinningFeatureEnabled) return
    const tokens = userVisibleTokensInfo.filter((token) => token.isErc721 || token.isNft)

    const getTokensSupport = async () => {
      const isTokenSupportedPromises = tokens.map(token => isTokenPinningSupported(token))
      const isTokenPinningSupportedResults =
        (await Promise.all(isTokenSupportedPromises)).map(res => res.result)
      const nfts = await Promise.all(tokens.map(async (token, idx) => {
        const canBePinned = (await areSupportedForPinning([token.logo]))
          && isTokenPinningSupportedResults[idx]
        return { canBePinned, token }
      }))
      const pinnable = nfts
        .filter(({ canBePinned }) => canBePinned)
        .map(({ token }) => token)
      if (!ignore) {
        setNonFungibleTokens(nfts)
        setPinnableNfts(pinnable)
      }
    }

    getTokensSupport()
      .catch(console.error)
    return () => {
      ignore = true
    }
  }, [isNftPinningFeatureEnabled, userVisibleTokensInfo, isTokenPinningSupported])

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
