// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useState } from 'react'

// Types / constants
import {
  BraveWallet,
  SerializableTransactionInfo,
} from '../../../../constants/types'

// Query hooks
import { useGetSwapStatusQuery } from '../../../../common/slices/api.slice'

// Utils
import {
  getTransactionMemo,
  getTransactionToAddress,
} from '../../../../utils/tx-utils'

const terminalSwapStatuses = new Set([
  BraveWallet.Gate3SwapStatusCode.kSuccess,
  BraveWallet.Gate3SwapStatusCode.kFailed,
  BraveWallet.Gate3SwapStatusCode.kRefunded,
])

// Providers whose swaps support status polling.
const needsStatusChecks = new Set([
  BraveWallet.SwapProvider.kNearIntents,
  BraveWallet.SwapProvider.kSquid,
])

export interface UseGate3SwapStatusResult {
  status?: BraveWallet.Gate3SwapStatus
  error?: BraveWallet.Gate3SwapError
  errorString?: string
  isLoading: boolean
  isEnabled: boolean
  refetch: () => void
}

/**
 * Hook to query Gate3 swap status for a transaction
 * @param transaction - The transaction info containing swap details
 * @returns Status information including current state, error, and loading state
 */
export function useGate3SwapStatus(
  transaction:
    | BraveWallet.TransactionInfo
    | SerializableTransactionInfo
    | null
    | undefined,
): UseGate3SwapStatusResult {
  // Extract swap info from transaction
  const swapInfo = transaction?.swapInfo

  // Extract deposit address and memo from transaction
  const depositAddress = transaction ? getTransactionToAddress(transaction) : ''
  const memoString = transaction ? getTransactionMemo(transaction) : ''
  const depositMemo = memoString
    ? Array.from(new TextEncoder().encode(memoString))
    : undefined

  // Only query for Gate3-supported providers with a valid route and tx hash
  const shouldQuery =
    !!transaction
    && !!swapInfo
    && swapInfo.routeId !== ''
    && transaction.txHash !== ''
    && needsStatusChecks.has(swapInfo.provider)

  // Build status params
  const statusParams: BraveWallet.Gate3SwapStatusParams | undefined =
    shouldQuery && swapInfo
      ? {
          routeId: swapInfo.routeId,
          txHash: transaction.txHash,
          sourceCoin: swapInfo.sourceCoin,
          sourceChainId: swapInfo.sourceChainId,
          destinationCoin: swapInfo.destinationCoin,
          destinationChainId: swapInfo.destinationChainId,
          depositAddress,
          depositMemo,
          provider: swapInfo.provider,
        }
      : undefined

  // Query the status with constant polling
  // The polling will continue until the transaction reaches a final state
  const [isTerminal, setIsTerminal] = useState(false)

  const { data, isLoading, isFetching, refetch } = useGetSwapStatusQuery(
    statusParams,
    {
      skip: !shouldQuery,
      pollingInterval: shouldQuery && !isTerminal ? 3000 : 0,
    },
  )

  const currentStatus = data?.response?.status
  if (
    !isTerminal
    && currentStatus !== undefined
    && terminalSwapStatuses.has(currentStatus)
  ) {
    setIsTerminal(true)
  }

  return {
    status: data?.response ?? undefined,
    error: data?.error ?? undefined,
    errorString: data?.errorString ?? undefined,
    isLoading: isLoading || isFetching,
    isEnabled: shouldQuery,
    refetch,
  }
}
