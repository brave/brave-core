/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Constants
import {
  BraveWallet,
  SerializableTransactionInfo
} from '../../constants/types'

// Utils
import {
  ParsedTransaction,
  parseTransactionWithPrices
} from '../../utils/tx-utils'
import { WalletSelectors } from '../selectors'

// Hooks
import { useUnsafeWalletSelector } from './use-safe-selector'
import { useGetSelectedChainQuery } from '../slices/api.slice'

export function useTransactionParser (
  transactionNetwork?: BraveWallet.NetworkInfo,
  gasFee?: string
) {
  // queries
  const { data: reduxSelectedNetwork } = useGetSelectedChainQuery()

  const fullTokenList = useUnsafeWalletSelector(WalletSelectors.fullTokenList)
  const visibleTokens = useUnsafeWalletSelector(
    WalletSelectors.userVisibleTokensInfo
  )
  const accounts = useUnsafeWalletSelector(WalletSelectors.accounts)
  const spotPrices = useUnsafeWalletSelector(
    WalletSelectors.transactionSpotPrices
  )

  const selectedNetwork = transactionNetwork || reduxSelectedNetwork

  return React.useCallback((tx: SerializableTransactionInfo): ParsedTransaction => {
    return parseTransactionWithPrices({
      accounts, // TODO - use accounts and balances registries here?
      fullTokenList,
      transactionNetwork: selectedNetwork,
      tx,
      userVisibleTokensList: visibleTokens,
      spotPrices,
      gasFee: gasFee ?? ''
    })
  }, [
    fullTokenList,
    visibleTokens,
    selectedNetwork,
    accounts,
    spotPrices,
    gasFee
  ])
}
