// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// constants
import { BraveWallet, SerializableTransactionInfo } from '../../constants/types'

// hooks
import {
  useGetAccountInfosRegistryQuery,
  useGetNetworkQuery,
  useGetSelectedAccountIdQuery,
  useGetTokensRegistryQuery,
  useGetTransactionsQuery,
  useGetUserTokensRegistryQuery,
  useGenerateReceiveAddressMutation
} from './api.slice'

// entities
import { selectAllAccountInfosFromQuery } from './entities/account-info.entity'

// utils
import {
  selectAllUserAssetsFromQueryResult,
  selectAllBlockchainTokensFromQueryResult,
  selectCombinedTokensList,
  selectCombinedTokensRegistry
} from '../slices/entities/blockchain-token.entity'
import {
  findAccountByAccountId,
  findAccountByAddress
} from '../../utils/account-utils'
import { getCoinFromTxDataUnion } from '../../utils/network-utils'
import { selectPendingTransactions } from './entities/transaction.entity'

export const useAccountsQuery = () => {
  return useGetAccountInfosRegistryQuery(undefined, {
    selectFromResult: (res) => ({
      isLoading: res.isLoading,
      accounts: selectAllAccountInfosFromQuery(res)
    })
  })
}

export const useAccountQuery = (
  accountId: BraveWallet.AccountId | undefined | typeof skipToken,
  opts?: { skip?: boolean }
) => {
  const skip = accountId === undefined || accountId === skipToken || opts?.skip
  return useGetAccountInfosRegistryQuery(skip ? skipToken : undefined, {
    skip: skip,
    selectFromResult: (res) => ({
      isLoading: res.isLoading,
      error: res.error,
      account:
        res.data && !skip
          ? findAccountByAccountId(accountId, res.data)
          : undefined
    })
  })
}

export const useAccountFromAddressQuery = (
  uniqueKeyOrAddress: string | undefined | typeof skipToken
) => {
  const skip =
    uniqueKeyOrAddress === undefined || uniqueKeyOrAddress === skipToken
  return useGetAccountInfosRegistryQuery(skip ? skipToken : undefined, {
    skip: skip,
    selectFromResult: (res) => ({
      isLoading: res.isLoading,
      error: res.error,
      account:
        res.data && !skip
          ? findAccountByAccountId(
              { uniqueKey: uniqueKeyOrAddress },
              res.data
            ) || findAccountByAddress(uniqueKeyOrAddress, res.data)
          : undefined
    })
  })
}

export const useSelectedAccountQuery = () => {
  const { data: selectedAccountId, isFetching: isLoadingSelectedAccountId } =
    useGetSelectedAccountIdQuery()

  const { account: selectedAccount, isLoading: isLoadingAccount } =
    useAccountQuery(selectedAccountId ?? skipToken)

  return {
    isLoading: isLoadingSelectedAccountId || isLoadingAccount,
    data: selectedAccount
  }
}

export const useGetCombinedTokensRegistryQuery = (
  arg?: undefined | typeof skipToken,
  opts?: { skip?: boolean }
) => {
  const { isLoadingUserTokens, userTokens } = useGetUserTokensRegistryQuery(
    arg || opts?.skip ? skipToken : undefined,
    {
      selectFromResult: (res) => ({
        isLoadingUserTokens: res.isLoading,
        userTokens: res.data
      })
    }
  )

  const { isLoadingKnownTokens, knownTokens } = useGetTokensRegistryQuery(
    undefined,
    {
      selectFromResult: (res) => ({
        isLoadingKnownTokens: res.isLoading,
        knownTokens: res.data
      }),
      skip: opts?.skip
    }
  )

  const combinedQuery = React.useMemo(() => {
    if (
      isLoadingUserTokens ||
      isLoadingKnownTokens ||
      !knownTokens ||
      !userTokens
    ) {
      return {
        isLoading: true,
        data: undefined
      }
    }
    const combinedRegistry = selectCombinedTokensRegistry(
      knownTokens,
      userTokens
    )
    return {
      isLoading: isLoadingUserTokens || isLoadingKnownTokens,
      data: combinedRegistry
    }
  }, [isLoadingKnownTokens, isLoadingUserTokens, userTokens, knownTokens])

  return combinedQuery
}

export const useGetCombinedTokensListQuery = (
  arg?: undefined | typeof skipToken
) => {
  const { isLoadingUserTokens, userTokens } = useGetUserTokensRegistryQuery(
    arg || undefined,
    {
      selectFromResult: (res) => ({
        isLoadingUserTokens: res.isLoading,
        userTokens: selectAllUserAssetsFromQueryResult(res)
      })
    }
  )

  const { isLoadingKnownTokens, knownTokens } = useGetTokensRegistryQuery(
    arg || undefined,
    {
      selectFromResult: (res) => ({
        isLoadingKnownTokens: res.isLoading,
        knownTokens: selectAllBlockchainTokensFromQueryResult(res)
      })
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

export const useReceiveAddressQuery = (
  accountId: BraveWallet.AccountId | undefined
) => {
  // state
  const [receiveAddress, setReceiveAddress] = React.useState<string>(
    accountId?.address || ''
  )
  const [isFetchingAddress, setIsFetchingAddress] =
    React.useState<boolean>(false)

  // mutations
  const [generateReceiveAddress] = useGenerateReceiveAddressMutation()

  // effects
  React.useEffect(() => {
    // skip fetching/polling if not needed
    if (accountId?.address) {
      return
    }

    let ignore = false

    const fetchAddress = async () => {
      if (accountId) {
        setIsFetchingAddress(true)
        const address = await generateReceiveAddress(accountId).unwrap()
        if (!ignore) {
          setReceiveAddress(address)
          setIsFetchingAddress(false)
        }
      }
    }

    fetchAddress()

    // poll for new address every BTC block (10 minutes)
    const intervalId = setInterval(fetchAddress, 1000 * 60 * 10)

    // cleanup
    return () => {
      ignore = true
      clearInterval(intervalId)
    }
  }, [accountId, generateReceiveAddress])

  return {
    receiveAddress,
    isFetchingAddress
  }
}

export const useGetIsRegistryTokenQuery = (
  arg:
    | {
        chainId: string
        address: string
      }
    | typeof skipToken
) => {
  return useGetTokensRegistryQuery(undefined, {
    selectFromResult: (res) => {
      if (arg === skipToken) {
        return {
          isLoading: res.isLoading
        }
      }

      const assetId = res.data?.idsByChainId[arg.chainId].find((id) =>
        id.toString().includes(arg?.address)
      )
      const asset = assetId ? res.data?.entities[assetId] : undefined

      return {
        isLoading: res.isLoading,
        isVerified: res.isLoading ? undefined : Boolean(asset)
      }
    }
  })
}
