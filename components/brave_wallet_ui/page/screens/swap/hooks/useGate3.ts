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
    async function (indicativeRoute: BraveWallet.Gate3SwapRoute) {
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
      if (needsAddressResolution || !toAccountAddress) {
        return
      }

      // If the route requires a firm quote, we need to fetch the transaction
      // from the backend. Otherwise, we can use the transaction params
      // embedded in the route.
      let firmRoute = indicativeRoute
      if (indicativeRoute.requiresFirmRoute) {
        let firmRouteResponse
        try {
          firmRouteResponse = await generateSwapTransaction(
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
          console.error(`Error getting Gate3 swap transaction: ${e}`)
          return makeGate3Error(`Error getting Gate3 swap transaction: ${e}`)
        }

        if (firmRouteResponse?.error) {
          return firmRouteResponse?.error
        }

        if (!firmRouteResponse?.response?.gate3Route) {
          return makeGate3Error('No transaction returned from Gate3')
        }

        firmRoute = firmRouteResponse.response.gate3Route
      }

      if (!firmRoute.transactionParams) {
        console.error('Gate3: No transaction params in firm route')
        return makeGate3Error('No transaction params in firm route')
      }

      try {
        await sendTransaction(firmRoute)
      } catch (e) {
        console.error(`Error creating Gate3 transaction: ${e}`)
        return makeGate3Error(`Error creating Gate3 transaction: ${e}`)
      }

      return undefined

      function makeGate3Error(message: string): BraveWallet.SwapErrorUnion {
        return {
          jupiterError: undefined,
          zeroExError: undefined,
          lifiError: undefined,
          squidError: undefined,
          gate3Error: {
            message,
            kind: BraveWallet.Gate3SwapErrorKind.kUnknown,
          },
        }
      }

      async function sendTransaction(firmRoute: BraveWallet.Gate3SwapRoute) {
        const { transactionParams } = firmRoute
        if (!transactionParams) {
          throw new Error('No transaction params found in firm route')
        }

        if (!fromNetwork) {
          throw new Error('Source network not found')
        }

        if (!fromAccount) {
          throw new Error('Source account not found')
        }

        if (!fromToken) {
          throw new Error('Source token not found')
        }

        if (!toToken) {
          throw new Error('Destination token not found')
        }

        if (!toAccountAddress) {
          throw new Error('Destination address not found')
        }

        if (transactionParams.evmTransactionParams) {
          const { data, to, value, gasLimit } =
            transactionParams.evmTransactionParams

          await sendEvmTransaction({
            fromAccount,
            to,
            value: new Amount(value).toHex(),
            data: hexStrToNumberArray(data),
            network: fromNetwork,
            gasLimit: new Amount(gasLimit).toHex(),
            swapInfo: {
              sourceCoin: fromToken.coin,
              sourceChainId: fromToken.chainId,
              sourceTokenAddress: fromToken.contractAddress,
              sourceAmount: firmRoute.sourceAmount,
              destinationCoin: toToken.coin,
              destinationChainId: toToken.chainId,
              destinationTokenAddress: toToken.contractAddress,
              destinationAmountMin: firmRoute.destinationAmountMin,
              destinationAmount: firmRoute.destinationAmount,
              recipient: toAccountAddress,
              provider: firmRoute.provider,
              routeId: firmRoute.id,
            } satisfies BraveWallet.SwapInfo,
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
              chainId: fromNetwork.chainId,
              accountId: fromAccount.accountId,
              txType: BraveWallet.TransactionType.SolanaSwap,
              encodedTransaction: versionedTransaction,
              swapInfo: {
                sourceCoin: fromToken.coin,
                sourceChainId: fromToken.chainId,
                sourceTokenAddress: fromToken.contractAddress,
                sourceAmount: firmRoute.sourceAmount,
                destinationCoin: toToken.coin,
                destinationChainId: toToken.chainId,
                destinationTokenAddress: toToken.contractAddress,
                destinationAmountMin: firmRoute.destinationAmountMin,
                destinationAmount: firmRoute.destinationAmount,
                recipient: toAccountAddress,
                provider: firmRoute.provider,
                routeId: firmRoute.id,
              } satisfies BraveWallet.SwapInfo,
            })
            return
          }

          // For SPL token transfers
          if (splTokenMint && splTokenAmount && decimals) {
            await sendSPLTransfer({
              network: fromNetwork,
              fromAccount,
              to,
              value: new Amount(splTokenAmount).toHex(),
              splTokenMintAddress: splTokenMint,
              decimals: Number(decimals),
              isCompressedNft: false,
              swapInfo: {
                sourceCoin: fromToken.coin,
                sourceChainId: fromToken.chainId,
                sourceTokenAddress: fromToken.contractAddress,
                sourceAmount: firmRoute.sourceAmount,
                destinationCoin: toToken.coin,
                destinationChainId: toToken.chainId,
                destinationTokenAddress: toToken.contractAddress,
                destinationAmountMin: firmRoute.destinationAmountMin,
                destinationAmount: firmRoute.destinationAmount,
                recipient: toAccountAddress,
                provider: firmRoute.provider,
                routeId: firmRoute.id,
              } satisfies BraveWallet.SwapInfo,
            })
            return
          }

          // For native SOL transfers
          await sendSolTransaction({
            network: fromNetwork,
            fromAccount,
            to,
            value: new Amount(lamports).toHex(),
            swapInfo: {
              sourceCoin: fromToken.coin,
              sourceChainId: fromToken.chainId,
              sourceTokenAddress: fromToken.contractAddress,
              sourceAmount: firmRoute.sourceAmount,
              destinationCoin: toToken.coin,
              destinationChainId: toToken.chainId,
              destinationTokenAddress: toToken.contractAddress,
              destinationAmountMin: firmRoute.destinationAmountMin,
              destinationAmount: firmRoute.destinationAmount,
              recipient: toAccountAddress,
              provider: firmRoute.provider,
              routeId: firmRoute.id,
            } satisfies BraveWallet.SwapInfo,
          })
          return
        }

        if (transactionParams.bitcoinTransactionParams) {
          const { to, value } = transactionParams.bitcoinTransactionParams

          await sendBtcTransaction({
            network: fromNetwork,
            fromAccount,
            to,
            value: new Amount(value).toHex(),
            sendingMaxAmount: false,
            swapInfo: {
              sourceCoin: fromToken.coin,
              sourceChainId: fromToken.chainId,
              sourceTokenAddress: fromToken.contractAddress,
              sourceAmount: firmRoute.sourceAmount,
              destinationCoin: toToken.coin,
              destinationChainId: toToken.chainId,
              destinationTokenAddress: toToken.contractAddress,
              destinationAmountMin: firmRoute.destinationAmountMin,
              destinationAmount: firmRoute.destinationAmount,
              recipient: toAccountAddress,
              provider: firmRoute.provider,
              routeId: firmRoute.id,
            } satisfies BraveWallet.SwapInfo,
          })
          return
        }

        if (transactionParams.cardanoTransactionParams) {
          const { to, value } = transactionParams.cardanoTransactionParams

          await sendCardanoTransaction({
            network: fromNetwork,
            fromAccount,
            to,
            value: new Amount(value).toHex(),
            sendingMaxAmount: false,
            swapInfo: {
              sourceCoin: fromToken.coin,
              sourceChainId: fromToken.chainId,
              sourceTokenAddress: fromToken.contractAddress,
              sourceAmount: firmRoute.sourceAmount,
              destinationCoin: toToken.coin,
              destinationChainId: toToken.chainId,
              destinationTokenAddress: toToken.contractAddress,
              destinationAmountMin: firmRoute.destinationAmountMin,
              destinationAmount: firmRoute.destinationAmount,
              recipient: toAccountAddress,
              provider: firmRoute.provider,
              routeId: firmRoute.id,
            } satisfies BraveWallet.SwapInfo,
          })
          return
        }

        if (transactionParams.zcashTransactionParams) {
          const { to, value } = transactionParams.zcashTransactionParams

          await sendZecTransaction({
            network: fromNetwork,
            fromAccount,
            to,
            value: new Amount(value).toHex(),
            sendingMaxAmount: false,
            useShieldedPool: false,
            memo: undefined,
            swapInfo: {
              sourceCoin: fromToken.coin,
              sourceChainId: fromToken.chainId,
              sourceTokenAddress: fromToken.contractAddress,
              sourceAmount: firmRoute.sourceAmount,
              destinationCoin: toToken.coin,
              destinationChainId: toToken.chainId,
              destinationTokenAddress: toToken.contractAddress,
              destinationAmountMin: firmRoute.destinationAmountMin,
              destinationAmount: firmRoute.destinationAmount,
              recipient: toAccountAddress,
              provider: firmRoute.provider,
              routeId: firmRoute.id,
            } satisfies BraveWallet.SwapInfo,
          })
        }

        throw new Error('Unsupported transaction params')
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
      sendSolanaSerializedTransaction,
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
