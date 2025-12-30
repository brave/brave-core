// Copyright (c) 2026 The Brave Authors. All rights reserved.
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
  useSendSolTransactionMutation,
  useSendSPLTransferMutation,
  useSendBtcTransactionMutation,
  useSendSolanaSerializedTransactionMutation,
} from '../../../../common/slices/api.slice'
import { useReceiveAddressQuery } from '../../../../common/slices/api.slice.extra'

export function useGate3(params: SwapParams) {
  const {
    fromAccount,
    fromToken,
    fromNetwork,
    fromAmount,
    toAccountId,
    toToken,
    toAmount,
    slippageTolerance,
  } = params

  // For UTXO-based accounts, the address field is empty, so we need to use
  // useReceiveAddressQuery to get the actual receive address for the toAccountId.
  const {
    receiveAddress: toAccountAddress,
    isFetchingAddress: isFetchingToAccountAddress,
  } = useReceiveAddressQuery(toAccountId)

  // Mutations
  const [sendEvmTransaction] = useSendEvmTransactionMutation()
  const [sendSolTransaction] = useSendSolTransactionMutation()
  const [sendSPLTransfer] = useSendSPLTransferMutation()
  const [sendSolanaSerializedTransaction] =
    useSendSolanaSerializedTransactionMutation()
  const [sendBtcTransaction] = useSendBtcTransactionMutation()
  const [generateSwapTransaction] = useGenerateSwapTransactionMutation()

  const exchange = useCallback(
    async function (route: BraveWallet.Gate3SwapRoute) {
      if (
        !fromAccount
        || !toAccountId
        || !fromToken
        || !toToken
        || !fromNetwork
      ) {
        return
      }

      if (!fromAmount && !toAmount) {
        return
      }

      const fromAmountWrapped = new Amount(fromAmount)
      const toAmountWrapped = new Amount(toAmount)
      const isFromAmountEmpty =
        fromAmountWrapped.isZero()
        || fromAmountWrapped.isNaN()
        || fromAmountWrapped.isUndefined()
      const isToAmountEmpty =
        toAmountWrapped.isZero()
        || toAmountWrapped.isNaN()
        || toAmountWrapped.isUndefined()

      if (isFromAmountEmpty && isToAmountEmpty) {
        return
      }

      if (isFetchingToAccountAddress || !toAccountAddress) {
        return
      }

      // If the route requires a firm quote, we need to fetch the transaction
      // from the backend. Otherwise, we can use the transaction params
      // embedded in the route.
      if (route.requiresFirmRoute) {
        let transactionResponse
        try {
          transactionResponse = await generateSwapTransaction(
            toMojoUnion(
              {
                gate3TransactionParams: {
                  fromAccountId: fromAccount.accountId,
                  fromChainId: fromToken.chainId,
                  fromAmount:
                    fromAmount
                    && new Amount(fromAmount)
                      .multiplyByDecimals(fromToken.decimals)
                      .format(),
                  fromToken: fromToken.contractAddress,
                  toAccountId: {
                    ...toAccountId,
                    address: toAccountAddress,
                  },
                  toChainId: toToken.chainId,
                  toAmount:
                    toAmount
                    && new Amount(toAmount)
                      .multiplyByDecimals(toToken.decimals)
                      .format(),
                  toToken: toToken.contractAddress,
                  slippagePercentage: slippageTolerance,
                  routePriority: BraveWallet.RoutePriority.kCheapest,
                  provider: BraveWallet.SwapProvider.kNearIntents,
                },
                jupiterTransactionParams: undefined,
                lifiTransactionParams: undefined,
                zeroExTransactionParams: undefined,
                squidTransactionParams: undefined,
              },
              'gate3TransactionParams',
            ),
          ).unwrap()
        } catch (e) {
          console.log(`Error getting Gate3 swap transaction: ${e}`)
        }

        if (transactionResponse?.error) {
          return transactionResponse?.error
        }

        if (!transactionResponse?.response?.gate3Transaction) {
          return
        }

        const transactionParams = transactionResponse.response.gate3Transaction

        try {
          await sendTransaction(transactionParams)
        } catch (e) {
          console.error(`Error creating Gate3 transaction: ${e}`)
        }

        return undefined
      }

      // Use the transaction params from the route directly
      if (!route.transactionParams) {
        console.error('Gate3: No transaction params in route')
        return
      }

      try {
        await sendTransaction(route.transactionParams)
      } catch (e) {
        console.error(`Error creating Gate3 transaction: ${e}`)
      }

      return undefined

      async function sendTransaction(
        transactionParams: BraveWallet.TransactionParamsUnion,
      ) {
        if (transactionParams.evmTransactionParams) {
          const { data, to, value } = transactionParams.evmTransactionParams

          await sendEvmTransaction({
            fromAccount: fromAccount!,
            to,
            value: new Amount(value).toHex(),
            data: hexStrToNumberArray(data),
            network: fromNetwork!,
            gasLimit: new Amount(21000).toHex(),
          })
          return
        }

        if (transactionParams.solanaTransactionParams) {
          const {
            to,
            value,
            splTokenMint,
            splTokenAmount,
            decimals,
            versionedTransaction,
          } = transactionParams.solanaTransactionParams

          if (versionedTransaction) {
            await sendSolanaSerializedTransaction({
              chainId: fromNetwork!.chainId,
              accountId: fromAccount!.accountId,
              txType: BraveWallet.TransactionType.SolanaSwap,
              encodedTransaction: versionedTransaction,
            })
            return
          }

          // For SPL token transfers
          if (splTokenMint && splTokenAmount && decimals) {
            await sendSPLTransfer({
              network: fromNetwork!,
              fromAccount: fromAccount!,
              to,
              value: new Amount(splTokenAmount).toHex(),
              splTokenMintAddress: splTokenMint,
              decimals: Number(decimals),
              isCompressedNft: false,
            })
            return
          }

          // For native SOL transfers
          await sendSolTransaction({
            network: fromNetwork!,
            fromAccount: fromAccount!,
            to,
            value: new Amount(value).toHex(),
          })
          return
        }

        if (transactionParams.bitcoinTransactionParams) {
          const { to, value } = transactionParams.bitcoinTransactionParams

          await sendBtcTransaction({
            network: fromNetwork!,
            fromAccount: fromAccount!,
            to,
            value: new Amount(value).toHex(),
            sendingMaxAmount: false,
          })
          return
        }
      }
    },
    [
      fromAccount,
      fromAmount,
      fromNetwork,
      fromToken,
      generateSwapTransaction,
      sendEvmTransaction,
      sendSolTransaction,
      sendSPLTransfer,
      sendBtcTransaction,
      slippageTolerance,
      toAccountId,
      toAccountAddress,
      toAmount,
      toToken,
    ],
  )

  return {
    exchange,
  }
}
