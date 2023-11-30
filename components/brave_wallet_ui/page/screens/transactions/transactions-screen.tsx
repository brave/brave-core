// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// types
import { BraveWallet } from '../../../constants/types'

// options
import { AllNetworksOption } from '../../../options/network-filter-options'

// utils
import { getLocale } from 'brave-ui'
import { accountInfoEntityAdaptorInitialState } from '../../../common/slices/entities/account-info.entity'
import { useAccountFromAddressQuery } from '../../../common/slices/api.slice.extra'
import {
  selectAllUserAssetsFromQueryResult,
  selectAllBlockchainTokensFromQueryResult
} from '../../../common/slices/entities/blockchain-token.entity'
import {
  networkEntityAdapter //
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
  useGetTransactionsQuery
} from '../../../common/slices/api.slice'
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../common/selectors'

// components
import {
  PortfolioTransactionItem //
} from '../../../components/desktop/portfolio_transaction_item/portfolio_transaction_item'
import {
  WalletPageWrapper //
} from '../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper'
import {
  ActivityPageHeader //
} from '../../../components/desktop/card-headers/activity_page_header'
import { SearchBar } from '../../../components/shared/search-bar'

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

const txListItemSkeletonProps: LoadingSkeletonStyleProps = {
  width: '100%',
  height: '60px',
  enableAnimation: true
}

export const TransactionsScreen: React.FC = () => {
  // routing
  const history = useHistory()

  // UI Selectors (safe)
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // state
  const [searchValue, setSearchValue] = React.useState<string>('')

  // route params
  const { address, chainId, chainCoinType } = React.useMemo(() => {
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

  const { account: foundAccountFromParam } = useAccountFromAddressQuery(
    address ?? undefined
  )

  const { data: knownTokensList } = useGetTokensRegistryQuery(undefined, {
    selectFromResult: (res) => ({
      isLoading: res.isLoading,
      data: selectAllBlockchainTokensFromQueryResult(res)
    })
  })

  const { data: userTokensList } = useGetUserTokensRegistryQuery(undefined, {
    selectFromResult: (res) => ({
      isLoading: res.isLoading,
      data: selectAllUserAssetsFromQueryResult(res)
    })
  })

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
      ? combinedTokensList.filter((token) => token.chainId === chainId)
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
  }, [searchValue, searchableTransactions])

  // render
  if (isLoadingAccounts || isLoadingTxsList) {
    return (
      <WalletPageWrapper
        wrapContentInBox={true}
        cardHeader={
          <ActivityPageHeader
            searchValue={searchValue}
            onSearchValueChange={(e) => setSearchValue(e.target.value)}
          />
        }
      >
        <Column fullHeight>
          <LoadingIcon
            opacity={100}
            size='50px'
            color='interactive05'
          />
        </Column>
      </WalletPageWrapper>
    )
  }

  return (
    <WalletPageWrapper
      wrapContentInBox={true}
      cardHeader={
        <ActivityPageHeader
          searchValue={searchValue}
          onSearchValueChange={(e) => setSearchValue(e.target.value)}
        />
      }
    >
      <>
        {isPanel && (
          <Column
            flex={1}
            style={{ minWidth: '100%' }}
          >
            <SearchBar
              placeholder={getLocale('braveWalletSearchText')}
              action={(e) => setSearchValue(e.target.value)}
              value={searchValue}
              isV2={true}
            />
            <VerticalSpacer space={24} />
          </Column>
        )}
        {isLoadingTxsList ? (
          <Column
            fullHeight
            fullWidth
          >
            <VerticalSpacer space={8} />
            <Skeleton {...txListItemSkeletonProps} />
            <VerticalSpacer space={8} />
            <Skeleton {...txListItemSkeletonProps} />
            <VerticalSpacer space={8} />
            <Skeleton {...txListItemSkeletonProps} />
            <VerticalSpacer space={8} />
          </Column>
        ) : (
          <>
            {txsForSelectedChain?.length === 0 && (
              <Column
                fullHeight
                gap={'24px'}
              >
                <VerticalSpacer space={14} />
                <Text
                  textSize='18px'
                  isBold
                >
                  {getLocale('braveWalletNoTransactionsYet')}
                </Text>
                <Text textSize='14px'>
                  {getLocale('braveWalletNoTransactionsYetDescription')}
                </Text>
              </Column>
            )}

            <Column
              fullWidth={true}
              fullHeight={true}
              justifyContent='flex-start'
            >
              {filteredTransactions.map((tx, i) => (
                <PortfolioTransactionItem
                  key={tx.id}
                  transaction={tx}
                />
              ))}
            </Column>

            {txsForSelectedChain &&
              txsForSelectedChain.length !== 0 &&
              filteredTransactions.length === 0 && (
                <Column fullHeight>
                  <Text textSize='14px'>
                    {getLocale('braveWalletConnectHardwareSearchNothingFound')}
                  </Text>
                </Column>
              )}
          </>
        )}
      </>
    </WalletPageWrapper>
  )
}

export default TransactionsScreen
