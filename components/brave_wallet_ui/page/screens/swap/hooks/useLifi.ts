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
  useSendEvmTransactionMutation,
  useSendSolanaSerializedTransactionMutation
} from '../../../../common/slices/api.slice'

export function useLifi({
  fromAccount,
  fromToken,
  fromNetwork,
  toAccountId,
  toToken,
  slippageTolerance
}: Omit<SwapParams, 'toAmount' | 'fromAmount'>) {
  // mutations
  const [sendEvmTransaction] = useSendEvmTransactionMutation()
  const [sendSolanaSerializedTransaction] =
    useSendSolanaSerializedTransactionMutation()
  const [generateSwapTransaction] = useGenerateSwapTransactionMutation()

  // methods
  const exchange = useCallback(
    async function (step: BraveWallet.LiFiStep) {
      if (
        !fromAccount ||
        !toAccountId ||
        !fromToken ||
        !toToken ||
        !fromNetwork
      ) {
        return
      }

      const { estimate } = step

      const { fromAmount, toAmount } = estimate

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
        const swapParamsUnion: BraveWallet.SwapTransactionParamsUnion = {
          lifiTransactionParams: step,
          jupiterTransactionParams: undefined,
          zeroExTransactionParams: undefined,
          squidTransactionParams: undefined
        }

        transactionResponse = await generateSwapTransaction(
          toMojoUnion(swapParamsUnion, 'lifiTransactionParams')
        ).unwrap()
      } catch (e) {
        console.log(`Error generating LiFi swap transaction: ${e}`)
      }

      if (transactionResponse?.error) {
        console.log('useLiFi.exchange: tx response error')
        console.log({
          err: transactionResponse?.error
        })
        return transactionResponse?.error
      }

      if (!transactionResponse?.response?.lifiTransaction) {
        return
      }

      const { evmTransaction, solanaTransaction } =
        transactionResponse.response.lifiTransaction

      try {
        if (evmTransaction) {
          await sendEvmTransaction({
            fromAccount,
            to: evmTransaction.to,
            value: new Amount(evmTransaction.value).toHex(),
            gasLimit: new Amount(evmTransaction.gasLimit).toHex(),
            data: hexStrToNumberArray(evmTransaction.data),
            network: fromNetwork
          }).unwrap()
          return undefined
        }

        if (solanaTransaction) {
          await sendSolanaSerializedTransaction({
            accountId: fromAccount.accountId,
            chainId: fromNetwork.chainId,
            encodedTransaction: solanaTransaction,
            // TODO: Bridge Type?
            txType: BraveWallet.TransactionType.SolanaSwap,
            sendOptions: {
              skipPreflight: {
                skipPreflight: true
              },
              maxRetries: {
                maxRetries: BigInt(3)
              },
              preflightCommitment: 'processed'
            }
          }).unwrap()
        }
      } catch (e) {
        // bubble up error
        console.error(`Error creating LiFi transaction: ${e}`)
      }

      return undefined
    },
    [
      fromAccount,
      fromNetwork,
      fromToken,
      generateSwapTransaction,
      sendEvmTransaction,
      sendSolanaSerializedTransaction,
      toAccountId,
      toToken
    ]
  )

  return {
    exchange
  }
}
