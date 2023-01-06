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
import { WalletSelectors } from '../../../common/selectors'
import {
  ParsedTransactionWithoutFiatValues,
  parseTransactionWithPrices
} from '../../../common/hooks/transaction-parser'
import { getNetworkFromTXDataUnion } from '../../../utils/network-utils'
import { accountInfoEntityAdaptorInitialState } from '../../../common/slices/entities/account-info.entity'
import { selectAllBlockchainTokensFromQueryResult, selectAllUserAssetsFromQueryResult } from '../../../common/slices/entities/blockchain-token.entity'
import { getEntitiesListFromEntityState } from '../../../utils/entities.utils'
import { networkEntityInitalState } from '../../../common/slices/entities/network.entity'

// hooks
import { useUnsafeWalletSelector } from '../../../common/hooks/use-safe-selector'
import {
  useGetAccountInfosRegistryQuery,
  useGetAllNetworksQuery,
  useGetTokensRegistryQuery,
  useGetUserTokensRegistryQuery
} from '../../../common/slices/api.slice'

// components
import { AccountFilterSelector } from '../../../components/desktop/account-filter-selector/account-filter-selector'
import { NetworkFilterSelector } from '../../../components/desktop/network-filter-selector/index'
import { PortfolioTransactionItem } from '../../../components/desktop'
import { SearchBar } from '../../../components/shared'

// styles
import { Column, LoadingIcon, Text, VerticalSpacer } from '../../../components/shared/style'
import { SearchAndFiltersRow } from './transaction-screen.styles'

interface Params {
  address?: string | null
  chainId?: string | null
  transactionId?: string | null
}

export const TransactionsScreen: React.FC = () => {
  // routing
  const history = useHistory()

  // state
  const [searchValue, setSearchValue] = React.useState<string>('')

  // redux
  const allTransactions = useUnsafeWalletSelector(WalletSelectors.transactions)
  const solFeeEstimates = useUnsafeWalletSelector(WalletSelectors.solFeeEstimates)
  const spotPrices = useUnsafeWalletSelector(WalletSelectors.transactionSpotPrices)
  const accounts = useUnsafeWalletSelector(WalletSelectors.accounts)

  // queries
  const {
    data: networksRegistry = networkEntityInitalState,
    isLoading: isLoadingNetworksRegistry
  } = useGetAllNetworksQuery(undefined)
  const {
    data: accountInfosRegistry = accountInfoEntityAdaptorInitialState,
    isLoading: isLoadingAccountInfosRegistry
  } = useGetAccountInfosRegistryQuery(undefined)

  const {
    fullTokenList,
    isLoading: isLoadingFullTokenList
  } = useGetTokensRegistryQuery(undefined, {
    selectFromResult: (result) => ({
      ...result,
      fullTokenList: selectAllBlockchainTokensFromQueryResult(result)
    })
  })

  const {
    userVisibleTokens,
    isLoading: isLoadingUserVisibleTokens
  } = useGetUserTokensRegistryQuery(undefined, {
    selectFromResult: result => ({
      ...result,
      userVisibleTokens: selectAllUserAssetsFromQueryResult(result)
    })
  })

  // computed / memos
  const isLoading =
    isLoadingAccountInfosRegistry ||
    isLoadingFullTokenList ||
    isLoadingNetworksRegistry ||
    isLoadingUserVisibleTokens

  const {
    address,
    chainId,
    transactionId
  } = React.useMemo(() => {
    const searchParams = new URLSearchParams(history.location.search)
    return {
      address: searchParams.get('address')?.toLowerCase(),
      chainId: searchParams.get('chainId')?.toLowerCase(),
      transactionId: searchParams.get('transactionId')?.toLowerCase()
    }
  }, [history.location.search])

  const networkList = React.useMemo(() => {
    return getEntitiesListFromEntityState(
      networksRegistry,
      networksRegistry?.ids
    )
  }, [networksRegistry])

  const foundAccountFromParam = address ? accountInfosRegistry.entities[address] : undefined
  const foundNetworkFromParam = chainId ? networksRegistry.entities[chainId] : undefined

  const txsForAccountAllChains = React.useMemo(() => {
    return foundAccountFromParam?.address
      ? allTransactions?.[foundAccountFromParam.address] || []
      : accounts.flatMap(a => allTransactions[a.address] || [])
  }, [foundAccountFromParam?.address, allTransactions, accounts])

  const nonRejectedTxs = React.useMemo(() => {
    return txsForAccountAllChains.filter(tx => tx.txStatus !== BraveWallet.TransactionStatus.Rejected)
  }, [txsForAccountAllChains])

  const nonRejectedTxsForSelectedChain = React.useMemo(() => {
    return chainId && chainId !== AllNetworksOption.chainId
      ? nonRejectedTxs.filter(tx => getNetworkFromTXDataUnion(tx.txDataUnion, networkList)?.chainId === chainId)
      : nonRejectedTxs
  }, [nonRejectedTxs, chainId])

  const parsedTransactions = React.useMemo(() => {
    // wait for data before attempting parsing
    if (isLoading) {
      return []
    }

    return nonRejectedTxsForSelectedChain.map(tx => parseTransactionWithPrices({
      accounts,
      fullTokenList,
      transactionNetwork: getNetworkFromTXDataUnion(tx.txDataUnion, networkList),
      tx,
      userVisibleTokensList: userVisibleTokens,
      solFeeEstimates,
      spotPrices
    }))
  }, [
    isLoading,
    nonRejectedTxsForSelectedChain,
    accounts,
    fullTokenList,
    networkList,
    userVisibleTokens,
    solFeeEstimates,
    spotPrices
  ])

  const filteredTransactions = React.useMemo(() => {
    return filterTransactionsBySearchValue(searchValue, parsedTransactions)
  }, [searchValue, parsedTransactions])

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
  if (isLoading) {
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
        />
        <NetworkFilterSelector
          selectedAccount={foundAccountFromParam || AllAccountsOption}
          selectedNetwork={foundNetworkFromParam || AllNetworksOption}
          onSelectNetwork={onSelectNetwork}
        />
      </SearchAndFiltersRow>

      {parsedTransactions.length === 0 &&
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

      {parsedTransactions.length !== 0 && filteredTransactions.length === 0 &&
        <Column fullHeight>
          <Text textSize='14px'>
            {getLocale('braveWalletConnectHardwareSearchNothingFound')}
          </Text>
        </Column>
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
