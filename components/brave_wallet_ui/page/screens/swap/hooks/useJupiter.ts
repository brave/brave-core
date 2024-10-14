// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useCallback } from 'react'

// Types
import { SwapParams } from '../constants/types'
import { BraveWallet } from '../../../../constants/types'

// Utils
import { toMojoUnion } from '../../../../utils/mojo-utils'

// Query hooks
import {
  useGenerateSwapTransactionMutation,
  useSendSolanaSerializedTransactionMutation
} from '../../../../common/slices/api.slice'

export function useJupiter(params: SwapParams) {
  const { fromAccount, fromNetwork } = params

  // Mutations
  const [generateSwapTransaction] = useGenerateSwapTransactionMutation()
  const [sendSolanaSerializedTransaction] =
    useSendSolanaSerializedTransactionMutation()

  const exchange = useCallback(
    async function (quote: BraveWallet.JupiterQuote) {
      if (!fromAccount || !fromNetwork) {
        return
      }

      // Perform data validation and early-exit
      if (quote.routePlan.length === 0) {
        return
      }

      if (fromNetwork.coin !== BraveWallet.CoinType.SOL) {
        return
      }

      let jupiterTransactionResponse
      try {
        jupiterTransactionResponse = await generateSwapTransaction(
          toMojoUnion(
            {
              jupiterTransactionParams: {
                chainId: fromNetwork.chainId,
                userPublicKey: fromAccount.address,
                quote
              },
              zeroExTransactionParams: undefined,
              lifiTransactionParams: undefined,
              squidTransactionParams: undefined
            },
            'jupiterTransactionParams'
          )
        ).unwrap()
      } catch (e) {
        console.log(`Error getting Jupiter swap transactions: ${e}`)
      }

      if (jupiterTransactionResponse?.error?.jupiterError) {
        return jupiterTransactionResponse.error
      }

      if (!jupiterTransactionResponse?.response?.jupiterTransaction) {
        return
      }

      const swapTransaction =
        jupiterTransactionResponse.response.jupiterTransaction

      try {
        await sendSolanaSerializedTransaction({
          encodedTransaction: swapTransaction,
          chainId: fromNetwork.chainId,
          accountId: fromAccount.accountId,
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
      } catch (e) {
        // Bubble up error
        console.error(`Error creating Solana transaction: ${e}`)
      }

      return undefined
    },
    [
      fromAccount,
      fromNetwork,
      generateSwapTransaction,
      sendSolanaSerializedTransaction
    ]
  )

  return {
    exchange
  }
}
