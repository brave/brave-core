// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { BraveWallet } from '../../constants/types'
import { reverseHttpifiedIpfsUrl, stripERC20TokenImageURL } from '../../utils/string-utils'
import { WalletSelectors } from '../selectors'
import { useUnsafeWalletSelector } from './use-safe-selector'

export function useNftPin () {
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

  return {
    nonFungibleTokens,
    pinnableNftsCount: pinnableNfts.length,
    pinnableNfts
  }
}
