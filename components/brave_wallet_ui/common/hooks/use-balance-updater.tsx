// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { WalletSelectors } from '../selectors'
import { useSafeWalletSelector } from './use-safe-selector'

// hooks
import { useLib } from './useLib'

export function useBalanceUpdater () {
  const { refreshBalances } = useLib()

  // Redux
  const isWalletLocked = useSafeWalletSelector(WalletSelectors.isWalletLocked)
  const isWalletCreated = useSafeWalletSelector(WalletSelectors.isWalletCreated)
  const hasInitialized = useSafeWalletSelector(WalletSelectors.hasInitialized)
  const dispatch = useDispatch()

  React.useEffect(() => {
    if (isWalletLocked || !isWalletCreated || !hasInitialized) {
      return
    }

    let subscribed = true
    const id = setInterval(() => {
      if (!subscribed) {
        return
      }
      dispatch(refreshBalances())
    }, 15000)

    // cleanup
    return () => {
      clearInterval(id)
      subscribed = false
    }
  }, [refreshBalances, isWalletLocked, isWalletCreated, hasInitialized])
}

export default useBalanceUpdater
