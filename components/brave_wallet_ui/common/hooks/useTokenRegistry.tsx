// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Redux
import { useUnsafeWalletSelector } from './use-safe-selector'
import { WalletSelectors } from '../selectors'

// Types
import { TokenRegistry, BraveWallet } from '../../constants/types'

// Hooks
import { useLib } from './'

// Utils
import { addLogoToToken } from '../../utils/asset-utils'

export function useTokenRegistry () {
  // Hooks
  const { getTokenList } = useLib()

  // Redux
  const networkList = useUnsafeWalletSelector(WalletSelectors.networkList)

  // Hook State
  const [tokenRegistry, setTokenRegistry] = React.useState<TokenRegistry>({})
  const [isLoading, setIsLoading] = React.useState<boolean>(true)

  React.useEffect(() => {
    let subscribed = true
    let registry = tokenRegistry
    Promise.all(networkList.map(async (network) => {
      await getTokenList(network).then(
        (result) => {
          const formattedListWithIcons = result.tokens.map((token) => {
            return addLogoToToken(token)
          })
          registry[network.chainId] = formattedListWithIcons
        }
      ).catch((error) => {
        if (!subscribed) {
          return
        }
        console.log(error)
        setIsLoading(false)
      })
    })).then(() => {
      if (!subscribed) {
        return
      }
      setTokenRegistry(registry)
      setIsLoading(false)
    })
    // cleanup
    return () => {
      subscribed = false
    }
  }, [tokenRegistry, networkList, getTokenList])

  // Creates a flat list of all tokens in the tokenRegistry
  const fullTokenListAllChains: BraveWallet.BlockchainToken[] = React.useMemo(() => {
    return Object.keys(tokenRegistry).length === 0 ? [] : networkList.map((network) => tokenRegistry[network.chainId]).flat(1)
  }, [tokenRegistry, networkList, Object.keys(tokenRegistry).length])

  return { tokenRegistry, fullTokenListAllChains, isLoading }
}

export default useTokenRegistry
