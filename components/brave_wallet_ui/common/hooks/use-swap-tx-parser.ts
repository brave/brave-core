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
        'chainId' | 'txType' | 'txDataUnion' | 'swapInfo'
      >
    | undefined
>(
  transaction: T
) => {
  const { data: sellNetwork } = useGetNetworkQuery(
    transaction?.swapInfo?.fromAsset === NATIVE_EVM_ASSET_CONTRACT_ADDRESS
      ? {
          chainId: transaction?.swapInfo.fromChainId,
          coin: transaction?.swapInfo.fromCoin
        }
      : skipToken
  )

  const { tokenInfo: sellTokenInfo } = useGetTokenInfo(
    transaction?.swapInfo &&
      transaction.swapInfo.fromAsset &&
      transaction.swapInfo.fromAsset !== NATIVE_EVM_ASSET_CONTRACT_ADDRESS
      ? {
          contractAddress: transaction.swapInfo.fromAsset,
          network: {
            chainId: transaction.swapInfo.fromChainId,
            coin: transaction.swapInfo.fromCoin
          }
        }
      : skipToken
  )

  const sellToken = React.useMemo(() => {
    if (sellNetwork) {
      return makeNetworkAsset(sellNetwork)
    }

    return sellTokenInfo
  }, [sellTokenInfo, sellNetwork])

  const { data: buyNetwork } = useGetNetworkQuery(
    transaction?.swapInfo?.toAsset === NATIVE_EVM_ASSET_CONTRACT_ADDRESS
      ? {
          chainId: transaction?.swapInfo.toChainId,
          coin: transaction?.swapInfo.toCoin
        }
      : skipToken
  )

  const { tokenInfo: buyTokenInfo } = useGetTokenInfo(
    transaction?.swapInfo &&
      transaction.swapInfo.toAsset &&
      transaction.swapInfo.toAsset !== NATIVE_EVM_ASSET_CONTRACT_ADDRESS
      ? {
          contractAddress: transaction.swapInfo.toAsset,
          network: {
            chainId: transaction.swapInfo.toChainId,
            coin: transaction.swapInfo.toCoin
          }
        }
      : skipToken
  )

  const buyToken = React.useMemo(() => {
    if (buyNetwork) {
      return makeNetworkAsset(buyNetwork)
    }

    return buyTokenInfo
  }, [buyTokenInfo, buyNetwork])

  const sellAmountWei = new Amount(transaction?.swapInfo?.fromAmount || '')
  const buyAmountWei = transaction?.swapInfo?.toAmount
    ? new Amount(transaction.swapInfo.toAmount)
    : sellAmountWei

  return {
    sellToken,
    sellAmountWei,
    buyToken,
    buyAmountWei,
    receiver: transaction?.swapInfo?.receiver || '',
    provider: transaction?.swapInfo?.provider
  }
}
