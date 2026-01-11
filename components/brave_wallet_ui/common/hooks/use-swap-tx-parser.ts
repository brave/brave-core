// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { skipToken } from '@reduxjs/toolkit/query/react'

// Types
import {
  BraveWallet,
  SerializableTransactionInfo,
  ParsedSwapInfo,
} from '../../constants/types'

// Utils
import Amount from '../../utils/amount'
import { NATIVE_EVM_ASSET_CONTRACT_ADDRESS } from '../constants/magics'

// Queries
import useGetTokenInfo from './use-get-token-info'

export const useSwapTransactionParser = <
  T extends
    | Pick<
        SerializableTransactionInfo | BraveWallet.TransactionInfo,
        'swapInfo'
      >
    | undefined,
>(
  transaction: T,
): ParsedSwapInfo => {
  const { tokenInfo: sourceToken } = useGetTokenInfo(
    transaction?.swapInfo
      ? {
          contractAddress:
            transaction.swapInfo.sourceTokenAddress
            !== NATIVE_EVM_ASSET_CONTRACT_ADDRESS
              ? transaction.swapInfo.sourceTokenAddress
              : '',
          network: {
            chainId: transaction.swapInfo.sourceChainId,
            coin: transaction.swapInfo.sourceCoin,
          },
        }
      : skipToken,
  )

  const { tokenInfo: destinationToken } = useGetTokenInfo(
    transaction?.swapInfo
      ? {
          contractAddress:
            transaction.swapInfo.destinationTokenAddress
            !== NATIVE_EVM_ASSET_CONTRACT_ADDRESS
              ? transaction.swapInfo.destinationTokenAddress
              : '',
          network: {
            chainId: transaction.swapInfo.destinationChainId,
            coin: transaction.swapInfo.destinationCoin,
          },
        }
      : skipToken,
  )

  const sourceAmount = new Amount(transaction?.swapInfo?.sourceAmount || '')
  const destinationAmount = new Amount(
    transaction?.swapInfo?.destinationAmount || '',
  )
  const destinationAmountMin = new Amount(
    transaction?.swapInfo?.destinationAmountMin || '',
  )

  return {
    sourceToken,
    sourceAmount,
    destinationToken,
    destinationAmount,
    destinationAmountMin,
    destinationAddress: transaction?.swapInfo?.recipient || '',
    provider: transaction?.swapInfo?.provider,
  } satisfies ParsedSwapInfo
}
