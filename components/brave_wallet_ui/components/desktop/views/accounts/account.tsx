// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  Redirect,
  useParams,
  useLocation,
  useHistory
} from 'react-router'
import { useDispatch } from 'react-redux'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Selectors
import { useSafeWalletSelector, useUnsafeWalletSelector } from '../../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../../common/selectors'

// Types
import {
  BraveWallet,
  CoinTypesMap,
  WalletRoutes,
  AccountModalTypes,
  AccountPageTabs
} from '../../../../constants/types'

// utils
import { getLocale } from '../../../../../common/locale'
import { sortTransactionByDate } from '../../../../utils/tx-utils'
import { getBalance } from '../../../../utils/balance-utils'
import { filterNetworksForAccount } from '../../../../utils/network-utils'
import { makeAccountRoute, makeAccountTransactionRoute } from '../../../../utils/routes-utils'

import Amount from '../../../../utils/amount'
import {
  getTokenPriceAmountFromRegistry
} from '../../../../utils/pricing-utils'
import { getPriceIdForToken } from '../../../../utils/api-utils'
import {
  selectAllVisibleUserAssetsFromQueryResult
} from '../../../../common/slices/entities/blockchain-token.entity'
import {
  getAssetIdKey
} from '../../../../utils/asset-utils'

// Styled Components
import {
  ControlsWrapper,
  AssetsWrapper,
  NFTsWrapper,
  TransactionsWrapper
} from './style'
import {
  Column,
  VerticalSpace,
  Text
} from '../../../shared/style'
import {
  EmptyTransactionsIcon
} from '../portfolio/style'
import {
  NftGrid
} from '../nfts/components/nfts.styles'

// Components
import {
  PortfolioTransactionItem
} from '../../portfolio-transaction-item/index'
import {
  PortfolioAssetItemLoadingSkeleton
} from '../../portfolio-asset-item/portfolio-asset-item-loading-skeleton'
import {
  PortfolioAssetItem
} from '../../portfolio-asset-item/index'
import {
  AccountDetailsHeader
} from '../../card-headers/account-details-header'
import {
  SegmentedControl
} from '../../../shared/segmented-control/segmented-control'
import {
  NFTGridViewItem
} from '../portfolio/components/nft-grid-view/nft-grid-view-item'
import {
  NftsEmptyState
} from '../nfts/components/nfts-empty-state/nfts-empty-state'
import {
  AddOrEditNftModal
} from '../../popup-modals/add-edit-nft-modal/add-edit-nft-modal'
import {
  WalletPageWrapper
} from '../../wallet-page-wrapper/wallet-page-wrapper'

// options
import {
  AccountDetailsOptions
} from '../../../../options/nav-options'

// Hooks
import { useScrollIntoView } from '../../../../common/hooks/use-scroll-into-view'
import {
  useGetDefaultFiatCurrencyQuery,
  useGetVisibleNetworksQuery,
  useGetUserTokensRegistryQuery,
  useGetTransactionsQuery,
  useGetTokenSpotPricesQuery
} from '../../../../common/slices/api.slice'
import {
  querySubscriptionOptions60s
} from '../../../../common/slices/constants'
import {
  useBalancesFetcher
} from '../../../../common/hooks/use-balances-fetcher'

// Actions
import { AccountsTabActions } from '../../../../page/reducers/accounts-tab-reducer'

export const Account = () => {
  // routing
  const { accountId: addressOrUniqueKey, selectedTab } =
    useParams<{ accountId: string, selectedTab?: string }>()
  const { hash: transactionID } = useLocation()
  const history = useHistory()

  // redux
  const dispatch = useDispatch()

  // unsafe selectors
  const accounts = useUnsafeWalletSelector(WalletSelectors.accounts)
  const selectedAccount = React.useMemo(() => {
    return accounts.find((account) =>
      account.accountId.uniqueKey === addressOrUniqueKey ||
      account.address.toLowerCase() === addressOrUniqueKey?.toLowerCase()
    )
  }, [accounts, addressOrUniqueKey])

  // queries
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()
  const { data: networkList = [] } = useGetVisibleNetworksQuery()
  const { userVisibleTokensInfo } = useGetUserTokensRegistryQuery(undefined, {
    selectFromResult: result => ({
      userVisibleTokensInfo: selectAllVisibleUserAssetsFromQueryResult(result)
    })
  })
  const { data: unsortedTransactionList = [] } = useGetTransactionsQuery(
    selectedAccount
      ? {
          accountId: selectedAccount.accountId,
          chainId: null,
          coinType: selectedAccount.accountId.coin
        }
      : skipToken,
    { skip: !selectedAccount }
  )

  // safe selectors
  const assetAutoDiscoveryCompleted = useSafeWalletSelector(WalletSelectors.assetAutoDiscoveryCompleted)

  // state
  const [showAddNftModal, setShowAddNftModal] = React.useState<boolean>(false)

  // custom hooks
  const scrollIntoView = useScrollIntoView()

  // memos
  const transactionList = React.useMemo(() => {
    return sortTransactionByDate(unsortedTransactionList, 'descending')
  }, [unsortedTransactionList])

  const accountsTokensList = React.useMemo(() => {
    if (!selectedAccount) {
      return []
    }
    // Since LOCALHOST's chainId is shared between coinType's
    // this check will make sure we are returning the correct
    // LOCALHOST asset for each account.
    const hasLocalHostNetwork = networkList.some(
      (network) =>
        network.chainId === BraveWallet.LOCALHOST_CHAIN_ID
        && network.coin === selectedAccount.accountId.coin
    )
    const coinName = CoinTypesMap[selectedAccount.accountId.coin]
    const localHostCoins = userVisibleTokensInfo.filter((token) => token.chainId === BraveWallet.LOCALHOST_CHAIN_ID)
    const accountsLocalHost = localHostCoins.find((token) => token.symbol.toUpperCase() === coinName)
    const chainList = filterNetworksForAccount(
      networkList,
      selectedAccount.accountId
    ).map((network) => network.chainId)
    const list =
      userVisibleTokensInfo.filter((token) => chainList.includes(token?.chainId ?? '') &&
        token.chainId !== BraveWallet.LOCALHOST_CHAIN_ID) ?? []
    if (
      accountsLocalHost &&
      hasLocalHostNetwork &&
      selectedAccount.accountId.keyringId !== BraveWallet.KeyringId.kFilecoin
    ) {
      return [...list, accountsLocalHost]
    }
    return list
  }, [userVisibleTokensInfo, selectedAccount, networkList])

  const {
    data: tokenBalancesRegistry,
    isLoading: isLoadingBalances
  } = useBalancesFetcher(selectedAccount && networkList
    ? {
        accounts: [selectedAccount],
        networks: networkList
      }
    : skipToken
  )

  const nonFungibleTokens = React.useMemo(
    () =>
      accountsTokensList.filter(({ isErc721, isNft }) => isErc721 || isNft)
        .filter(token =>
          new Amount(getBalance(selectedAccount?.accountId, token, tokenBalancesRegistry))
            .gt(0)),
    [accountsTokensList, selectedAccount, tokenBalancesRegistry]
  )

  const fungibleTokens = React.useMemo(
    () => accountsTokensList.filter(({ isErc721, isErc1155, isNft }) =>
      !(isErc721 || isErc1155 || isNft)),
    [accountsTokensList]
  )

  const tokenPriceIds = React.useMemo(() =>
    fungibleTokens
      .filter(token =>
        new Amount(getBalance(selectedAccount?.accountId, token, tokenBalancesRegistry))
          .gt(0))
      .map(getPriceIdForToken),
    [fungibleTokens, selectedAccount, tokenBalancesRegistry]
  )

  const {
    data: spotPriceRegistry,
    isLoading: isLoadingSpotPrices
  } = useGetTokenSpotPricesQuery(
    tokenPriceIds.length && defaultFiatCurrency
      ? { ids: tokenPriceIds, toCurrency: defaultFiatCurrency }
      : skipToken,
    querySubscriptionOptions60s
  )

  const routeOptions = React.useMemo(() => {
    if (!selectedAccount) return []
    return AccountDetailsOptions.map((option) => {
      return {
        ...option,
        route: makeAccountRoute(
          selectedAccount,
          option.route as AccountPageTabs
        ) as WalletRoutes
      }
    })
  }, [selectedAccount])

  // Methods
  const onRemoveAccount = React.useCallback(() => {
    if (selectedAccount) {
      dispatch(
        AccountsTabActions.setAccountToRemove({
          accountId: selectedAccount.accountId,
          name: selectedAccount.name
        })
      )
    }
  }, [selectedAccount, dispatch])

  const onClickMenuOption = React.useCallback(
    (option: AccountModalTypes) => {
      if (option === 'remove') {
        onRemoveAccount()
        return
      }
      dispatch(AccountsTabActions.setShowAccountModal(true))
      dispatch(AccountsTabActions.setAccountModalType(option))
      dispatch(AccountsTabActions.setSelectedAccount(selectedAccount))
    }, [
    onRemoveAccount,
    selectedAccount
  ])

  const checkIsTransactionFocused = React.useCallback((id: string): boolean => {
    if (transactionID !== '') {
      return transactionID.replace('#', '') === id
    }
    return false
  }, [transactionID])

  const handleScrollIntoView = React.useCallback((id: string, ref: HTMLDivElement | null) => {
    if (checkIsTransactionFocused(id)) {
      scrollIntoView(ref)
    }
  }, [checkIsTransactionFocused, scrollIntoView])

  const onSelectAsset = React.useCallback(
    (asset: BraveWallet.BlockchainToken) => {
      if (asset.contractAddress === '') {
        history.push(
          `${WalletRoutes.PortfolioAssets //
          }/${asset.chainId //
          }/${asset.symbol}`
        )
        return
      }
      if (asset.isErc721 || asset.isNft || asset.isErc1155) {
        history.push(
          `${WalletRoutes.PortfolioNFTs //
          }/${asset.chainId //
          }/${asset.contractAddress //
          }/${asset.tokenId}`
        )
        return
      }
      history.push(
        `${WalletRoutes.PortfolioAssets //
        }/${asset.chainId //
        }/${asset.contractAddress}`
      )
    }, [])

  // redirect (asset not found)
  if (!selectedAccount) {
    return <Redirect to={WalletRoutes.Accounts} />
  }

  if (transactionID && !selectedTab) {
    return (
      <Redirect
        to={makeAccountTransactionRoute(selectedAccount, transactionID)}
      />
    )
  }

  // render
  return (
    <WalletPageWrapper
      wrapContentInBox
      hideDivider={true}
      noCardPadding={true}
      cardHeader={
        <AccountDetailsHeader
          account={selectedAccount}
          onClickMenuOption={onClickMenuOption}
          tokenBalancesRegistry={tokenBalancesRegistry}
        />
      }
    >
      <ControlsWrapper
        fullWidth={true}
      >
        <SegmentedControl
          navOptions={routeOptions}
        />
      </ControlsWrapper>

      {selectedTab === AccountPageTabs.AccountAssetsSub && (
        <AssetsWrapper
          fullWidth={true}
        >
          {fungibleTokens.map((asset) =>
            <PortfolioAssetItem
              key={getAssetIdKey(asset)}
              action={() => onSelectAsset(asset)}
              account={selectedAccount}
              assetBalance={
                isLoadingBalances
                  ? ''
                  : getBalance(
                    selectedAccount.accountId,
                    asset,
                    tokenBalancesRegistry
                  )
              }
              token={asset}
              spotPrice={
                spotPriceRegistry && !isLoadingSpotPrices
                  ? getTokenPriceAmountFromRegistry(spotPriceRegistry, asset)
                    .format()
                  : !spotPriceRegistry &&
                    !isLoadingSpotPrices &&
                    !isLoadingBalances
                    ? '0'
                    : ''
              }
            />
          )}
          {!assetAutoDiscoveryCompleted &&
            <PortfolioAssetItemLoadingSkeleton />
          }
        </AssetsWrapper>
      )}

      {selectedTab === AccountPageTabs.AccountNFTsSub && (
        <NFTsWrapper
          fullWidth={true}
        >
          {nonFungibleTokens?.length !== 0 ? (
            <NftGrid>
              {nonFungibleTokens?.map((nft: BraveWallet.BlockchainToken) =>
                <NFTGridViewItem
                  key={getAssetIdKey(nft)}
                  token={nft}
                  onSelectAsset={() => onSelectAsset(nft)}
                  isTokenHidden={false}
                  isTokenSpam={false}
                  accounts={accounts}
                  networks={networkList}
                />
              )}
            </NftGrid>
          ) : (
            <NftsEmptyState
              onImportNft={() => setShowAddNftModal(true)}
            />
          )}
        </NFTsWrapper>
      )}

      {selectedTab === AccountPageTabs.AccountTransactionsSub && (
        <>
          {transactionList.length !== 0 ? (
            <TransactionsWrapper
              fullWidth={true}
            >
              {transactionList.map((transaction) =>
                <PortfolioTransactionItem
                  key={transaction?.id}
                  transaction={transaction}
                  displayAccountName={false}
                  ref={(ref) => handleScrollIntoView(transaction.id, ref)}
                  isFocused={checkIsTransactionFocused(transaction.id)}
                />
              )}
            </TransactionsWrapper>
          ) : (
            <Column
              alignItems='center'
              justifyContent='center'
              fullHeight={true}
            >
              <EmptyTransactionsIcon />
              <Text
                textColor='text01'
                textSize='16px'
                isBold={true}
              >
                {getLocale('braveWalletNoTransactionsYet')}
              </Text>
              <VerticalSpace space='10px' />
              <Text
                textSize='14px'
                textColor='text03'
                isBold={false}
              >
                {getLocale('braveWalletNoTransactionsYetDescription')}
              </Text>
            </Column>
          )}
        </>
      )}

      {
        showAddNftModal && (
          <AddOrEditNftModal
            onClose={() => setShowAddNftModal(false)}
            onHideForm={() => setShowAddNftModal(false)}
          />
        )
      }
    </WalletPageWrapper >
  )
}

export default Account
