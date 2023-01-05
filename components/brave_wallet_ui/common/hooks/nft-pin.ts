// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { reverseHttpifiedIpfsUrl, stripERC20TokenImageURL } from '../../utils/string-utils'
import { WalletSelectors } from '../selectors'
import { useUnsafeWalletSelector } from './use-safe-selector'
import { useLib } from './useLib'

export function useNftPin () {
  const [pinnableNftsCount, setPinnableNftsCount] = React.useState<number>(0)
  const userVisibleTokensInfo = useUnsafeWalletSelector(
    WalletSelectors.userVisibleTokensInfo
  )

  // custom hooks
  const { getTokenPinningStatus } = useLib()

  const nonFungibleTokens = React.useMemo(() => {
    let pinnableCount = 0
    const tokens = userVisibleTokensInfo.filter(
      (token) => token.isErc721 || token.isNft
    )
    const nfts = tokens.map(token => {
      const canBePinned = reverseHttpifiedIpfsUrl(stripERC20TokenImageURL(token.logo)).startsWith('ipfs://')
      if (canBePinned) {
        pinnableCount += 1
      }
      return { canBePinned, token }
    })
    setPinnableNftsCount(pinnableCount)
    return nfts
  }, [userVisibleTokensInfo])

  React.useEffect(() => {
    const getPinningStatus = async () => {
      const promises = nonFungibleTokens.map(({ token }) => {
        return getTokenPinningStatus(token)
      })
      const results = await Promise.all(promises)
      console.log(results)
      // const nftPinStatus = results.map((result, index) => {
      //   return {
      //     ...result,
      //     ...nonFungibleTokens[index]
      //   }
      // })
    }

    getPinningStatus()
  }, [nonFungibleTokens])

  return {
    nonFungibleTokens,
    pinnableNftsCount
  }
}
