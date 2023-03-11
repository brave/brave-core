// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// types
import {
  BraveWallet,
  WalletAccountType,
  WalletRoutes
} from '../../../constants/types'

// options
import { AllNetworksOption } from '../../../options/network-filter-options'
import { AllAccountsOption } from '../../../options/account-filter-options'

// utils
import { getLocale } from 'brave-ui'
import {
  AccountInfoEntity,
  accountInfoEntityAdaptor,
  accountInfoEntityAdaptorInitialState,
  selectAllAccountInfosFromQuery
} from '../../../common/slices/entities/account-info.entity'
import {
  networkEntityAdapter,
  networkEntityInitialState
} from '../../../common/slices/entities/network.entity'
import { ParsedTransactionWithoutFiatValues } from '../../../utils/tx-utils'
import {
  combineTransactionRegistries,
  transactionEntityInitialState,
  TransactionEntityState
} from '../../../common/slices/entities/transaction.entity'
import { getEntitiesListFromEntityState } from '../../../utils/entities.utils'

// hooks
import {
  useGetAccountInfosRegistryQuery,
  useGetAllNetworksQuery,
  useLazyGetAllTransactionsForAddressCoinTypeQuery
} from '../../../common/slices/api.slice'

// components
import { AccountFilterSelector } from '../../../components/desktop/account-filter-selector/account-filter-selector'
import { NetworkFilterSelector } from '../../../components/desktop/network-filter-selector/index'
import { PortfolioTransactionItem } from '../../../components/desktop'
import { SearchBar } from '../../../components/shared'

// styles
import { Column, LoadingIcon, Text, VerticalSpacer } from '../../../components/shared/style'
import {
  LoadingSkeletonStyleProps,
  Skeleton
} from '../../../components/shared/loading-skeleton/styles'
import { SearchAndFiltersRow } from './transaction-screen.styles'

interface Params {
  address?: string | null
  chainId?: string | null
  transactionId?: string | null
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
  const [
    combinedTxEntityState,
    setCombinedTxEntityState
  ] = React.useState<TransactionEntityState>(transactionEntityInitialState)
  const [isLoadingTxsList, setIsLoadingTxsList] = React.useState<boolean>(false)

  // queries
  const {
    data: networksRegistry = networkEntityInitialState,
    isLoading: isLoadingNetworksRegistry
  } = useGetAllNetworksQuery(undefined)

  const {
    data: accountInfosRegistry = accountInfoEntityAdaptorInitialState,
    isLoading: isLoadingAccounts,
    accounts
  } = useGetAccountInfosRegistryQuery(undefined, {
    selectFromResult: res => ({
      ...res,
      accounts: selectAllAccountInfosFromQuery(res)
    })
  })

  const [fetchAllTransactionsForAddressCoinType] = useLazyGetAllTransactionsForAddressCoinTypeQuery()

  // computed / memos
  const isLoadingDeps =
    isLoadingAccounts ||
    isLoadingNetworksRegistry

  const {
    address,
    chainId,
    transactionId
  } = React.useMemo(() => {
    const searchParams = new URLSearchParams(history.location.search)
    return {
      address: searchParams.get('address'),
      chainId: searchParams.get('chainId'),
      transactionId: searchParams.get('transactionId')
    }
  }, [history.location.search])

  const foundAccountFromParam = address ? accountInfosRegistry.entities[
    accountInfoEntityAdaptor.selectId({ address })
  ] : undefined

  const foundNetworkFromParam = chainId
    ? chainId === AllNetworksOption.chainId
      ? AllNetworksOption
      : networksRegistry.entities[networkEntityAdapter.selectId({ chainId })]
    : undefined

  const fetchTxsForAccounts = React.useCallback((accounts: Array<Pick<AccountInfoEntity, 'address' | 'coin'>>) => {
    setIsLoadingTxsList(true)

    Promise.all(
      accounts.map(({ coin, address }) => {
        return fetchAllTransactionsForAddressCoinType({
          address: address,
          coinType: coin
        }).unwrap()
      }
    ))
    .then(registries => {
      const combinedRegistry: TransactionEntityState =
        combineTransactionRegistries(transactionEntityInitialState, registries)

      setCombinedTxEntityState(combinedRegistry)
      setIsLoadingTxsList(false)
    })
    .catch(error => {
      console.error(error)
      // stop loading if a error other than empty tokens list was thrown
      if (!error?.toString().includes('Unable to fetch Tokens Registry')) {
        setIsLoadingTxsList(false)
        return
      }
      // retry when browser is idle
      requestIdleCallback(() => {
        fetchTxsForAccounts(accounts)
      })
    })
  }, [
    fetchAllTransactionsForAddressCoinType
  ])

  React.useEffect(() => {
    if (isLoadingDeps) {
      return
    }

    if (
      foundAccountFromParam?.address &&
      foundAccountFromParam?.coin !== undefined
    ) {
      fetchTxsForAccounts([{
        address: foundAccountFromParam.address,
        coin: foundAccountFromParam.coin
      }])
      return
    }

    // get txs for all accounts
    fetchTxsForAccounts(accounts)
  }, [
    isLoadingDeps,
    accounts,
    fetchTxsForAccounts,
    foundAccountFromParam?.address,
    foundAccountFromParam?.coin
  ])

  const txIdsForSelectedChain = React.useMemo(() => {
    return chainId && chainId !== AllNetworksOption.chainId
      ? combinedTxEntityState.idsByChainId[chainId] ?? []
      : combinedTxEntityState.ids ?? []
  }, [combinedTxEntityState.idsByChainId, combinedTxEntityState.ids, chainId])

  const txsForSelectedChain = React.useMemo(() => {
    return getEntitiesListFromEntityState(
      combinedTxEntityState,
      txIdsForSelectedChain
    )
  }, [combinedTxEntityState, txIdsForSelectedChain])

  const filteredTransactions = React.useMemo(() => {
    return filterTransactionsBySearchValue(searchValue, txsForSelectedChain)
  }, [searchValue, txsForSelectedChain])

  // methods
  const onSelectAccount = React.useCallback(({ address }: WalletAccountType): void => {
    history.push(
      updatePageParams({
        address: address || undefined,
        chainId: AllNetworksOption.chainId, // reset chains filter on account select
        transactionId
      })
    )
  }, [history, transactionId])

  const onSelectNetwork = React.useCallback(({ chainId }: BraveWallet.NetworkInfo) => {
    history.push(
      updatePageParams({
        address: foundAccountFromParam?.address || AllAccountsOption.id,
        chainId,
        transactionId
      })
    )
  }, [history, foundAccountFromParam?.address, transactionId])

  // render
  if (isLoadingDeps) {
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
  transactionId
}: Params) => {
  const params = new URLSearchParams()
  if (address) {
    params.append('address', address)
  }
  if (chainId) {
    params.append('chainId', chainId)
  }
  if (transactionId) {
    params.append('transactionId', transactionId)
  }
  const paramsString = params.toString()

  return `${WalletRoutes.Activity}${paramsString ? `?${paramsString}` : ''}`
}

const findTokenBySearchValue = (
  searchValue: string,
  token?: BraveWallet.BlockchainToken
) => {
  if (!token) {
    return false
  }

  return (
    token.name.toLowerCase().includes(searchValue) ||
    token.symbol.toLowerCase().includes(searchValue) ||
    token.contractAddress.toLowerCase().includes(searchValue)
  )
}

const filterTransactionsBySearchValue = <T extends ParsedTransactionWithoutFiatValues>(
  searchValue: string,
  txsForFilteredChain: T[]
): T[] => {
  const lowerCaseSearchValue = searchValue.toLowerCase()
  return searchValue === ''
    ? txsForFilteredChain
    : txsForFilteredChain.filter((tx) => (
      // Tokens
      findTokenBySearchValue(lowerCaseSearchValue, tx.token) ||
      // Buy Token
      findTokenBySearchValue(lowerCaseSearchValue, tx.buyToken) ||
      // Sell Token
      findTokenBySearchValue(lowerCaseSearchValue, tx.sellToken) ||
      // ERC721 NFTs
      findTokenBySearchValue(lowerCaseSearchValue, tx.erc721BlockchainToken) ||
      // Sender
      tx.sender.toLowerCase().includes(lowerCaseSearchValue) ||
      tx.senderLabel.toLowerCase().includes(lowerCaseSearchValue) ||
      // Receiver
      tx.recipient.toLowerCase().includes(lowerCaseSearchValue) ||
      tx.recipientLabel.toLowerCase().includes(lowerCaseSearchValue) ||
      // Intent
      tx.intent.toLowerCase().includes(lowerCaseSearchValue) ||
      // Hash
      tx.hash.toLowerCase().includes(lowerCaseSearchValue) ||
      // Account Address
      tx.accountAddress.toLowerCase().includes(lowerCaseSearchValue) ||
      // Approval Target
      tx.approvalTarget && tx.approvalTarget.toLowerCase().includes(lowerCaseSearchValue) ||
      tx.approvalTargetLabel && tx.approvalTargetLabel.toLowerCase().includes(lowerCaseSearchValue) ||
      tx.originInfo?.eTldPlusOne.toLowerCase().includes(searchValue.toLowerCase()) ||
      tx.originInfo?.originSpec.toLowerCase().includes(searchValue.toLowerCase())
    ))
}
