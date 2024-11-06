// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useCallback } from 'react'

// Types / constants
import { SwapParams } from '../constants/types'
import { BraveWallet } from '../../../../constants/types'

// Utils
import Amount from '../../../../utils/amount'
import { hexStrToNumberArray } from '../../../../utils/hex-utils'
import { toMojoUnion } from '../../../../utils/mojo-utils'

// Query hooks
import {
  useGenerateSwapTransactionMutation,
  useSendEvmTransactionMutation
} from '../../../../common/slices/api.slice'

export function useZeroEx(params: SwapParams) {
  const {
    fromAccount,
    fromToken,
    fromNetwork,
    fromAmount,
    toAccountId,
    toToken,
    toAmount,
    slippageTolerance
  } = params

  // Mutations
  const [sendEvmTransaction] = useSendEvmTransactionMutation()
  const [generateSwapTransaction] = useGenerateSwapTransactionMutation()

  const exchange = useCallback(
    async function () {
      if (
        !fromAccount ||
        !toAccountId ||
        !fromToken ||
        !toToken ||
        !fromNetwork
      ) {
        return
      }

      if (!fromAmount && !toAmount) {
        return
      }

      const fromAmountWrapped = new Amount(fromAmount)
      const toAmountWrapped = new Amount(toAmount)
      const isFromAmountEmpty =
        fromAmountWrapped.isZero() ||
        fromAmountWrapped.isNaN() ||
        fromAmountWrapped.isUndefined()
      const isToAmountEmpty =
        toAmountWrapped.isZero() ||
        toAmountWrapped.isNaN() ||
        toAmountWrapped.isUndefined()

      if (isFromAmountEmpty && isToAmountEmpty) {
        return
      }

      let transactionResponse
      try {
        transactionResponse = await generateSwapTransaction(
          toMojoUnion(
            {
              zeroExTransactionParams: {
                fromAccountId: fromAccount.accountId,
                fromChainId: fromToken.chainId,
                fromAmount:
                  fromAmount &&
                  new Amount(fromAmount)
                    .multiplyByDecimals(fromToken.decimals)
                    .format(),
                fromToken: fromToken.contractAddress,
                toAccountId,
                toChainId: toToken.chainId,
                toAmount:
                  toAmount &&
                  new Amount(toAmount)
                    .multiplyByDecimals(toToken.decimals)
                    .format(),
                toToken: toToken.contractAddress,
                slippagePercentage: slippageTolerance,
                routePriority: BraveWallet.RoutePriority.kCheapest,
                provider: BraveWallet.SwapProvider.kZeroEx
              },
              jupiterTransactionParams: undefined,
              lifiTransactionParams: undefined,
              squidTransactionParams: undefined
            },
            'zeroExTransactionParams'
          )
        ).unwrap()
      } catch (e) {
        console.log(`Error getting 0x swap quote: ${e}`)
      }

      if (transactionResponse?.error) {
        return transactionResponse?.error
      }

      if (!transactionResponse?.response?.zeroExTransaction) {
        return
      }

      const { data, to, value, gas } =
        transactionResponse.response.zeroExTransaction

      try {
        await sendEvmTransaction({
          fromAccount,
          to,
          value: new Amount(value).toHex(),
          gasLimit: new Amount(gas).toHex(),
          data: hexStrToNumberArray(data),
          network: fromNetwork
        })
      } catch (e) {
        // bubble up error
        console.error(`Error creating 0x transaction: ${e}`)
      }

      return undefined
    },
    [
      fromAccount,
      fromAmount,
      fromNetwork,
      fromToken,
      generateSwapTransaction,
      sendEvmTransaction,
      slippageTolerance,
      toAccountId,
      toAmount,
      toToken
    ]
  )

  return {
    exchange
  }
}
