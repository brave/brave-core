// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// types
import {
  BraveWallet,
  WalletRoutes
} from '../../../constants/types'

// options
import { AllNetworksOption } from '../../../options/network-filter-options'
import { AllAccountsOption } from '../../../options/account-filter-options'

// utils
import { getLocale } from 'brave-ui'
import {
  accountInfoEntityAdaptor,
  accountInfoEntityAdaptorInitialState
} from '../../../common/slices/entities/account-info.entity'
import {
  selectAllUserAssetsFromQueryResult,
  selectAllBlockchainTokensFromQueryResult
} from '../../../common/slices/entities/blockchain-token.entity'
import {
  networkEntityAdapter
} from '../../../common/slices/entities/network.entity'
import {
  filterTransactionsBySearchValue,
  makeSearchableTransaction
} from '../../../utils/search-utils'

// hooks
import {
  useGetAccountInfosRegistryQuery,
  useGetNetworksRegistryQuery,
  useGetUserTokensRegistryQuery,
  useGetTokensRegistryQuery,
  useGetTransactionsQuery,
} from '../../../common/slices/api.slice'

// components
import { AccountFilterSelector } from '../../../components/desktop/account-filter-selector/account-filter-selector'
import { NetworkFilterSelector } from '../../../components/desktop/network-filter-selector/index'
import {
  PortfolioTransactionItem //
} from '../../../components/desktop/portfolio-transaction-item/index'
import { SearchBar } from '../../../components/shared/search-bar/index'

// styles
import {
  Column,
  LoadingIcon,
  Text,
  VerticalSpacer
} from '../../../components/shared/style'
import {
  LoadingSkeletonStyleProps,
  Skeleton
} from '../../../components/shared/loading-skeleton/styles'
import { SearchAndFiltersRow } from './transaction-screen.styles'

interface Params {
  address?: string | null
  chainId?: string | null
  chainCoinType?: BraveWallet.CoinType | null
}

const txListItemSkeletonProps: LoadingSkeletonStyleProps = {
  width: '100%',
  height: '60px',
  enableAnimation: true
}

export const TransactionsScreen: React.FC = () => {
  // routing
  const history = useHistory()

  // state
  const [searchValue, setSearchValue] = React.useState<string>('')

  // route params
  const {
    address,
    chainId,
    chainCoinType
  } = React.useMemo(() => {
    const searchParams = new URLSearchParams(history.location.search)
    return {
      address: searchParams.get('address'),
      chainId: searchParams.get('chainId'),
      chainCoinType:
        Number(searchParams.get('chainCoinType')) || BraveWallet.CoinType.ETH
    }
  }, [history.location.search])

  // queries
  const {
    data: accountInfosRegistry = accountInfoEntityAdaptorInitialState,
    isLoading: isLoadingAccounts
  } = useGetAccountInfosRegistryQuery(undefined)
  const foundAccountFromParam = address
    ? accountInfosRegistry.entities[
        accountInfoEntityAdaptor.selectIdByAddress(address)
      ]
    : undefined

  const { data: knownTokensList } = useGetTokensRegistryQuery(undefined, {
    selectFromResult: (res) => ({
      isLoading: res.isLoading,
      data: selectAllBlockchainTokensFromQueryResult(res)
    })
  })

  const { data: userTokensList } = useGetUserTokensRegistryQuery(
    undefined,
    {
      selectFromResult: (res) => ({
        isLoading: res.isLoading,
        data: selectAllUserAssetsFromQueryResult(res)
      })
    }
  )

  const { data: networksRegistry } = useGetNetworksRegistryQuery()

  const specificNetworkFromParam =
    chainId &&
    chainId !== AllNetworksOption.chainId &&
    chainCoinType !== undefined &&
    networksRegistry
      ? networksRegistry.entities[
          networkEntityAdapter.selectId({
            chainId,
            coin: chainCoinType
          })
        ]
      : undefined

  const foundNetworkFromParam = chainId
    ? chainId === AllNetworksOption.chainId
      ? AllNetworksOption
      : specificNetworkFromParam
    : undefined

  const { data: txsForSelectedChain = [], isLoading: isLoadingTxsList } =
    useGetTransactionsQuery(
      foundAccountFromParam
        ? {
            accountId: foundAccountFromParam.accountId,
            coinType: foundAccountFromParam.accountId.coin,
            chainId: chainId !== AllNetworksOption.chainId ? chainId : null
          }
        : {
            accountId: null,
            chainId:
              foundNetworkFromParam?.chainId === AllNetworksOption.chainId
                ? null
                : foundNetworkFromParam?.chainId || null,
            coinType:
              foundNetworkFromParam?.chainId !== AllNetworksOption.chainId
                ? foundNetworkFromParam?.coin || null
                : null
          }
    )

  const combinedTokensList = React.useMemo(() => {
    return userTokensList.concat(knownTokensList)
  }, [userTokensList, knownTokensList])

  const combinedTokensListForSelectedChain = React.useMemo(() => {
    return chainId && chainId !== AllNetworksOption.chainId
      ? combinedTokensList.filter(token => token.chainId === chainId)
      : combinedTokensList
  }, [chainId, combinedTokensList])

  const searchableTransactions = React.useMemo(() => {
    return txsForSelectedChain.map((tx) => {
      return makeSearchableTransaction(
        tx,
        combinedTokensListForSelectedChain,
        networksRegistry,
        accountInfosRegistry
      )
    })
  }, [
    txsForSelectedChain,
    combinedTokensListForSelectedChain,
    networksRegistry,
    accountInfosRegistry
  ])

  const filteredTransactions = React.useMemo(() => {
    if (searchValue.trim() === '') {
      return searchableTransactions
    }

    return filterTransactionsBySearchValue(
      searchableTransactions,
      searchValue.toLowerCase()
    )
  }, [
    searchValue,
    searchableTransactions
  ])

  // methods
  const onSelectAccount = React.useCallback(
    ({ accountId }: BraveWallet.AccountInfo): void => {
      history.push(
        updatePageParams({
          address: accountId.address,
          // reset chains filter on account select
          chainId: AllNetworksOption.chainId,
          chainCoinType: accountId.coin
        })
      )
    },
    [history]
  )

  const onSelectNetwork = React.useCallback(
    ({ chainId, coin }: BraveWallet.NetworkInfo) => {
      history.push(
        updatePageParams({
          address: foundAccountFromParam?.address,
          chainId,
          chainCoinType: coin
        })
      )
    },
    [history, foundAccountFromParam?.address]
  )

  // render
  if (isLoadingAccounts || isLoadingTxsList) {
    return <Column fullHeight>
      <LoadingIcon opacity={100} size='50px' color='interactive05' />
    </Column>
  }

  return (
    <>
      <SearchAndFiltersRow>
        <Column flex={1} style={{ minWidth: '25%' }} alignItems='flex-start'>
          <SearchBar
            placeholder={getLocale('braveWalletSearchText')}
            action={(e) => setSearchValue(e.target.value)}
            value={searchValue}
          />
        </Column>
        <AccountFilterSelector
          selectedAccount={foundAccountFromParam || AllAccountsOption}
          onSelectAccount={onSelectAccount}
          selectedNetwork={foundNetworkFromParam || AllNetworksOption}
        />
        <NetworkFilterSelector
          selectedAccount={foundAccountFromParam || AllAccountsOption}
          selectedNetwork={foundNetworkFromParam || AllNetworksOption}
          onSelectNetwork={onSelectNetwork}
        />
      </SearchAndFiltersRow>

      {isLoadingTxsList
        ? <Column fullHeight fullWidth>
            <VerticalSpacer space={8} />
            <Skeleton {...txListItemSkeletonProps} />
            <VerticalSpacer space={8} />
            <Skeleton {...txListItemSkeletonProps} />
            <VerticalSpacer space={8} />
            <Skeleton {...txListItemSkeletonProps} />
            <VerticalSpacer space={8} />
          </Column>
        : <>
            {txsForSelectedChain?.length === 0 &&
              <Column fullHeight gap={'24px'}>
                <VerticalSpacer space={14} />
                <Text textSize='18px' isBold>
                  {getLocale('braveWalletNoTransactionsYet')}
                </Text>
                <Text textSize='14px'>
                  {getLocale('braveWalletNoTransactionsYetDescription')}
                </Text>
              </Column>
            }

            {filteredTransactions.map(tx =>
              <PortfolioTransactionItem
                key={tx.id}
                displayAccountName
                transaction={tx}
              />
            )}

            {txsForSelectedChain &&
              txsForSelectedChain.length !== 0 &&
              filteredTransactions.length === 0 &&
              <Column fullHeight>
                <Text textSize='14px'>
                  {getLocale('braveWalletConnectHardwareSearchNothingFound')}
                </Text>
              </Column>
            }
          </>
      }
    </>
  )
}

export default TransactionsScreen

const updatePageParams = ({
  address,
  chainId,
  chainCoinType
}: Params) => {
  const params = new URLSearchParams()
  if (address) {
    params.append('address', address)
  }
  if (chainId) {
    params.append('chainId', chainId)
  }
  if (chainCoinType) {
    params.append('chainCoinType', chainCoinType.toString())
  }
  const paramsString = params.toString()

  return `${WalletRoutes.Activity}${paramsString ? `?${paramsString}` : ''}`
}
