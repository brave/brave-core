// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Types
import { BraveWallet, SerializableTransactionInfo } from '../../constants/types'

// Utils
import Amount from '../../utils/amount'
import { NATIVE_EVM_ASSET_CONTRACT_ADDRESS } from '../constants/magics'
import { makeNetworkAsset } from '../../options/asset-options'

// Queries
import { useGetNetworkQuery } from '../slices/api.slice'
import useGetTokenInfo from './use-get-token-info'

export const useSwapTransactionParser = <
  T extends
    | Pick<
        SerializableTransactionInfo | BraveWallet.TransactionInfo,
        'chainId' | 'txType' | 'txDataUnion' | 'swapInfoDeprecated'
      >
    | undefined,
>(
  transaction: T,
) => {
  const { data: sellNetwork } = useGetNetworkQuery(
    transaction?.swapInfoDeprecated?.fromAsset
      === NATIVE_EVM_ASSET_CONTRACT_ADDRESS
      ? {
          chainId: transaction?.swapInfoDeprecated.fromChainId,
          coin: transaction?.swapInfoDeprecated.fromCoin,
        }
      : skipToken,
  )

  const { tokenInfo: sellTokenInfo } = useGetTokenInfo(
    transaction?.swapInfoDeprecated
      && transaction.swapInfoDeprecated.fromAsset
      && transaction.swapInfoDeprecated.fromAsset
        !== NATIVE_EVM_ASSET_CONTRACT_ADDRESS
      ? {
          contractAddress: transaction.swapInfoDeprecated.fromAsset,
          network: {
            chainId: transaction.swapInfoDeprecated.fromChainId,
            coin: transaction.swapInfoDeprecated.fromCoin,
          },
        }
      : skipToken,
  )

  const sellToken = React.useMemo(() => {
    if (sellNetwork) {
      return makeNetworkAsset(sellNetwork)
    }

    return sellTokenInfo
  }, [sellTokenInfo, sellNetwork])

  const { data: buyNetwork } = useGetNetworkQuery(
    transaction?.swapInfoDeprecated?.toAsset
      === NATIVE_EVM_ASSET_CONTRACT_ADDRESS
      ? {
          chainId: transaction?.swapInfoDeprecated.toChainId,
          coin: transaction?.swapInfoDeprecated.toCoin,
        }
      : skipToken,
  )

  const { tokenInfo: buyTokenInfo } = useGetTokenInfo(
    transaction?.swapInfoDeprecated
      && transaction.swapInfoDeprecated.toAsset
      && transaction.swapInfoDeprecated.toAsset
        !== NATIVE_EVM_ASSET_CONTRACT_ADDRESS
      ? {
          contractAddress: transaction.swapInfoDeprecated.toAsset,
          network: {
            chainId: transaction.swapInfoDeprecated.toChainId,
            coin: transaction.swapInfoDeprecated.toCoin,
          },
        }
      : skipToken,
  )

  const buyToken = React.useMemo(() => {
    if (buyNetwork) {
      return makeNetworkAsset(buyNetwork)
    }

    return buyTokenInfo
  }, [buyTokenInfo, buyNetwork])

  const sellAmountWei = new Amount(
    transaction?.swapInfoDeprecated?.fromAmount || '',
  )
  const buyAmountWei = transaction?.swapInfoDeprecated?.toAmount
    ? new Amount(transaction.swapInfoDeprecated.toAmount)
    : sellAmountWei

  return {
    sellToken,
    sellAmountWei,
    buyToken,
    buyAmountWei,
    receiver: transaction?.swapInfoDeprecated?.receiver || '',
    provider: transaction?.swapInfoDeprecated?.provider,
  }
}
