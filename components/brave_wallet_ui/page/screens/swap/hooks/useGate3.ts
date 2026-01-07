// Copyright (c) 2025 The Brave Authors. All rights reserved.
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
  useSendCardanoTransactionMutation,
  useSendZecTransactionMutation,
  useSendSolanaSerializedTransactionMutation,
} from '../../../../common/slices/api.slice'

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
    fromAccountAddress,
    toAccountAddress,
    needsAddressResolution,
  } = params

  // Mutations
  const [sendEvmTransaction] = useSendEvmTransactionMutation()
  const [sendSolTransaction] = useSendSolTransactionMutation()
  const [sendSPLTransfer] = useSendSPLTransferMutation()
  const [sendSolanaSerializedTransaction] =
    useSendSolanaSerializedTransactionMutation()
  const [sendBtcTransaction] = useSendBtcTransactionMutation()
  const [sendCardanoTransaction] = useSendCardanoTransactionMutation()
  const [sendZecTransaction] = useSendZecTransactionMutation()
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

      // For UTXO accounts (like BTC), wait for the receive addresses to be
      // fetched.
      if (needsAddressResolution) {
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
                  fromAccountId: {
                    ...fromAccount.accountId,
                    // Use fetched address for UTXO accounts where address field
                    // is empty
                    address:
                      fromAccount.accountId.address || fromAccountAddress || '',
                  },
                  fromChainId: fromToken.chainId,
                  fromAmount:
                    fromAmount
                    && new Amount(fromAmount)
                      .multiplyByDecimals(fromToken.decimals)
                      .format(),
                  fromToken: fromToken.contractAddress,
                  toAccountId: {
                    ...toAccountId,
                    // Use fetched address for UTXO accounts where address field
                    // is empty
                    address: toAccountId.address || toAccountAddress || '',
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
        transactionParams: BraveWallet.Gate3SwapTransactionParamsUnion,
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
            lamports,
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
            value: new Amount(lamports).toHex(),
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

        if (transactionParams.cardanoTransactionParams) {
          const { to, value } = transactionParams.cardanoTransactionParams

          await sendCardanoTransaction({
            network: fromNetwork!,
            fromAccount: fromAccount!,
            to,
            value: new Amount(value).toHex(),
            sendingMaxAmount: false,
          })
          return
        }

        if (transactionParams.zcashTransactionParams) {
          const { to, value } = transactionParams.zcashTransactionParams

          await sendZecTransaction({
            network: fromNetwork!,
            fromAccount: fromAccount!,
            to,
            value: new Amount(value).toHex(),
            sendingMaxAmount: false,
            useShieldedPool: false,
            memo: undefined,
          })
          return
        }
      }
    },
    [
      fromAccount,
      fromAccountAddress,
      fromAmount,
      fromNetwork,
      fromToken,
      generateSwapTransaction,
      needsAddressResolution,
      sendEvmTransaction,
      sendSolTransaction,
      sendSPLTransfer,
      sendBtcTransaction,
      sendCardanoTransaction,
      sendZecTransaction,
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
