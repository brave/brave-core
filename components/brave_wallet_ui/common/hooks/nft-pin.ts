// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { BraveWallet } from '../../constants/types'
import { reverseHttpifiedIpfsUrl, stripERC20TokenImageURL } from '../../utils/string-utils'
import { LOCAL_STORAGE_KEYS } from '../constants/local-storage-keys'
import { WalletSelectors } from '../selectors'
import { useUnsafeWalletSelector } from './use-safe-selector'

export function useNftPin () {
  const [isIpfsBannerVisible, setIsIpfsBannerVisible] = React.useState<boolean>(
    localStorage.getItem(LOCAL_STORAGE_KEYS.IS_IPFS_BANNER_HIDDEN) === 'false' ||
    localStorage.getItem(LOCAL_STORAGE_KEYS.IS_IPFS_BANNER_HIDDEN) === null
  )

  const userVisibleTokensInfo = useUnsafeWalletSelector(
    WalletSelectors.userVisibleTokensInfo
  )

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

  const onToggleShowIpfsBanner = React.useCallback(() => {
    setIsIpfsBannerVisible(prev => {
      window.localStorage.setItem(
        LOCAL_STORAGE_KEYS.IS_IPFS_BANNER_HIDDEN,
        prev ? 'false' : 'true'
      )
      return !prev
    })
  }, [])

  return {
    nonFungibleTokens,
    pinnableNftsCount: pinnableNfts.length,
    pinnableNfts,
    isIpfsBannerVisible,
    onToggleShowIpfsBanner
  }
}
