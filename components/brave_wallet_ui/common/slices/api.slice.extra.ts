// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// constants
import type {
  BraveWallet,
  SerializableTransactionInfo
} from '../../constants/types'

// hooks
import {
  useGetAccountInfosRegistryQuery,
  useGetNetworkQuery,
  useGetSelectedAccountIdQuery,
  useGetTokensRegistryQuery,
  useGetTransactionsQuery,
  useGetUserTokensRegistryQuery,
} from './api.slice'

// entities
import {
  accountInfoEntityAdaptor,
  accountInfoEntityAdaptorInitialState
} from './entities/account-info.entity'

// utils
import {
  selectAllUserAssetsFromQueryResult,
  selectAllBlockchainTokensFromQueryResult,
  selectCombinedTokensList
} from '../slices/entities/blockchain-token.entity'
import {
  findAccountFromRegistry
} from '../../utils/account-utils'
import { getCoinFromTxDataUnion } from '../../utils/network-utils'
import { selectPendingTransactions } from './entities/transaction.entity'

export const useAccountQuery = (
  address: string | typeof skipToken,
  opts?: { skip?: boolean }
) => {
  return useGetAccountInfosRegistryQuery(
    address === skipToken ? skipToken : undefined,
    {
      skip: address === skipToken || opts?.skip,
      selectFromResult: (res) => ({
        isLoading: res.isLoading,
        error: res.error,
        account:
          res.data && address !== skipToken
            ? findAccountFromRegistry(address, res.data)
            : undefined
      })
    }
  )
}

export const useSelectedAccountQuery = () => {
  const {
    data: accountInfosRegistry = accountInfoEntityAdaptorInitialState,
    isLoading: isLoadingAccounts
  } = useGetAccountInfosRegistryQuery(undefined)

  const { data: selectedAccountId, isLoading: isLoadingSelectedAccountId } =
  useGetSelectedAccountIdQuery(isLoadingAccounts ? skipToken : undefined)

  const selectedAccount = selectedAccountId
    ? accountInfosRegistry.entities[
        accountInfoEntityAdaptor.selectIdByAccountId(selectedAccountId)
      ]
    : undefined

  return {
    isLoading: isLoadingAccounts || isLoadingSelectedAccountId,
    data: selectedAccount
  }
}

export const useGetCombinedTokensListQuery = (
  arg?: undefined,
  opts?: { skip?: boolean }
) => {
  const { isLoadingUserTokens, userTokens } = useGetUserTokensRegistryQuery(
    undefined,
    {
      selectFromResult: (res) => ({
        isLoadingUserTokens: res.isLoading,
        userTokens: selectAllUserAssetsFromQueryResult(res)
      }),
      skip: opts?.skip
    }
  )

  const { isLoadingKnownTokens, knownTokens } = useGetTokensRegistryQuery(
    undefined,
    {
      selectFromResult: (res) => ({
        isLoadingKnownTokens: res.isLoading,
        knownTokens: selectAllBlockchainTokensFromQueryResult(res)
      }),
      skip: opts?.skip
    }
  )

  const combinedQuery = React.useMemo(() => {
    if (isLoadingUserTokens || isLoadingKnownTokens) {
      return {
        isLoading: true,
        data: [] as BraveWallet.BlockchainToken[]
      }
    }
    const combinedList = selectCombinedTokensList(knownTokens, userTokens)
    return {
      isLoading: isLoadingUserTokens || isLoadingKnownTokens,
      data: combinedList
    }
  }, [isLoadingKnownTokens, isLoadingUserTokens, userTokens, knownTokens])

  return combinedQuery
}

export const useTransactionQuery = (
  txID: string | typeof skipToken,
  opts?: { skip?: boolean }
) => {
  return useGetTransactionsQuery(
    txID === skipToken
      ? skipToken
      : {
          address: null,
          chainId: null,
          coinType: null
        },
    {
      skip: txID === skipToken || opts?.skip,
      selectFromResult: (res) => ({
        isLoading: res.isLoading,
        transaction: res.data?.find((tx) => tx.id === txID)
      })
    }
  )
}

export const useTransactionsNetworkQuery = <
  T extends
    | Pick<
        SerializableTransactionInfo | BraveWallet.TransactionInfo,
        'chainId' | 'txDataUnion'
      >
    | undefined
    | typeof skipToken
>(
  transaction: T
) => {
  // queries
  return useGetNetworkQuery(
    transaction === skipToken
      ? skipToken
      : transaction
      ? {
          chainId: transaction.chainId,
          coin: getCoinFromTxDataUnion(transaction.txDataUnion)
        }
      : skipToken
  )
}

const emptyPendingTxs: SerializableTransactionInfo[] = []

export const usePendingTransactionsQuery = (
  arg: Parameters<typeof useGetTransactionsQuery>[0]
) => {
  return useGetTransactionsQuery(arg, {
    selectFromResult: (res) => ({
      isLoading: res.isLoading,
      transactions: res.data || emptyPendingTxs,
      pendingTransactions: res.data
        ? selectPendingTransactions(res.data)
        : emptyPendingTxs
    })
  })
}
