// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { Redirect, useParams, useLocation, useHistory } from 'react-router'
import { useDispatch } from 'react-redux'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Selectors
import {
  useSafeUISelector,
  useSafeWalletSelector, //
} from '../../../../common/hooks/use-safe-selector'
import { UISelectors, WalletSelectors } from '../../../../common/selectors'

// Types
import {
  BraveWallet,
  CoinTypesMap,
  WalletRoutes,
  AccountModalTypes,
  AccountPageTabs,
  SupportedTestNetworks,
  BitcoinTestnetKeyringIds,
  ZCashTestnetKeyringIds,
  CardanoTestnetKeyringIds,
} from '../../../../constants/types'

// utils
import { getLocale } from '../../../../../common/locale'
import { sortTransactionByDate } from '../../../../utils/tx-utils'
import {
  getBalance,
  formatTokenBalanceWithSymbol,
} from '../../../../utils/balance-utils'
import { filterNetworksForAccount } from '../../../../utils/network-utils'
import {
  makeAccountRoute,
  makeAccountTransactionRoute,
  makePortfolioAssetRoute,
  openTab,
} from '../../../../utils/routes-utils'

import Amount from '../../../../utils/amount'
import {
  getTokenPriceAmountFromRegistry,
  computeFiatAmount,
  getPriceIdForToken,
} from '../../../../utils/pricing-utils'
import {
  selectAllVisibleUserAssetsFromQueryResult, //
} from '../../../../common/slices/entities/blockchain-token.entity'
import { getAssetIdKey, isTokenWatchOnly } from '../../../../utils/asset-utils'

// Styled Components
import {
  ControlsWrapper,
  AssetsWrapper,
  NFTsWrapper,
  TransactionsWrapper,
  EmptyStateWrapper,
  SyncAlert,
  SyncAlertWrapper,
} from './style'
import { Column, Row, VerticalSpace, Text } from '../../../shared/style'
import { EmptyTransactionsIcon } from '../portfolio/style'
import { NftGrid } from '../nfts/components/nfts.styles'

// Components
import {
  PortfolioTransactionItem, //
} from '../../portfolio_transaction_item/portfolio_transaction_item'
import {
  PortfolioAssetItemLoadingSkeleton, //
} from '../../portfolio-asset-item/portfolio-asset-item-loading-skeleton'
import { PortfolioAssetItem } from '../../portfolio-asset-item/index'
import {
  AccountDetailsHeader, //
} from '../../card-headers/account-details-header'
import {
  SegmentedControl, //
} from '../../../shared/segmented_control/segmented_control'
import {
  NFTGridViewItem, //
} from '../portfolio/components/nft-grid-view/nft-grid-view-item'
import {
  NftsEmptyState, //
} from '../nfts/components/nfts-empty-state/nfts-empty-state'
import {
  AddOrEditNftModal, //
} from '../../popup-modals/add-edit-nft-modal/add-edit-nft-modal'
import {
  WalletPageWrapper, //
} from '../../wallet-page-wrapper/wallet-page-wrapper'
import {
  EmptyTokenListState, //
} from '../portfolio/components/empty-token-list-state/empty-token-list-state'
import {
  ViewOnBlockExplorerModal, //
} from '../../popup-modals/view_on_block_explorer_modal/view_on_block_explorer_modal'
import { ZCashSyncModal } from '../../popup-modals/zcash_sync_modal/zcash_sync_modal'

// options
import { AccountDetailsOptions } from '../../../../options/nav-options'

// Hooks
import useInterval from '../../../../common/hooks/interval'
import { useScrollIntoView } from '../../../../common/hooks/use-scroll-into-view'
import {
  useGetDefaultFiatCurrencyQuery,
  useGetVisibleNetworksQuery,
  useGetUserTokensRegistryQuery,
  useGetTransactionsQuery,
  useGetTokenSpotPricesQuery,
  useStartShieldSyncMutation,
  useGetChainTipStatusQuery,
  useGetZCashAccountInfoQuery,
  useStopShieldSyncMutation,
  useGetZCashBalanceQuery,
  useClearChainTipStatusCacheMutation,
} from '../../../../common/slices/api.slice'
import {
  querySubscriptionOptions60s, //
} from '../../../../common/slices/constants'
import {
  useBalancesFetcher, //
} from '../../../../common/hooks/use-balances-fetcher'
import { useExplorer } from '../../../../common/hooks/explorer'
import {
  useIsAccountSyncing, //
} from '../../../../common/hooks/use_is_account_syncing'

// Actions
import { AccountsTabActions } from '../../../../page/reducers/accounts-tab-reducer'
import { useAccountsQuery } from '../../../../common/slices/api.slice.extra'

const INDIVIDUAL_TESTNET_ACCOUNT_KEYRING_IDS = [
  ...BitcoinTestnetKeyringIds,
  BraveWallet.KeyringId.kFilecoinTestnet,
  ...ZCashTestnetKeyringIds,
  ...CardanoTestnetKeyringIds,
]

const removedNFTsRouteOptions = AccountDetailsOptions.filter(
  (option) => option.id !== 'nfts',
)

const coinSupportsNFTs = (coin: BraveWallet.CoinType) => {
  return [BraveWallet.CoinType.ETH, BraveWallet.CoinType.SOL].includes(coin)
}

const coinSupportsAssets = (coin: BraveWallet.CoinType) => {
  return [BraveWallet.CoinType.ETH, BraveWallet.CoinType.SOL].includes(coin)
}

export const Account = () => {
  // safe selectors
  const assetAutoDiscoveryCompleted = useSafeWalletSelector(
    WalletSelectors.assetAutoDiscoveryCompleted,
  )
  const isZCashShieldedTransactionsEnabled = useSafeWalletSelector(
    WalletSelectors.isZCashShieldedTransactionsEnabled,
  )
  const isPanel = useSafeUISelector(UISelectors.isPanel)
  const isAndroid = useSafeUISelector(UISelectors.isAndroid)
  // mutations
  const [startShieldSync] = useStartShieldSyncMutation()
  const [stopShieldSync] = useStopShieldSyncMutation()

  // routing
  const { accountId: addressOrUniqueKey, selectedTab } = useParams<{
    accountId: string
    selectedTab?: string
  }>()
  const { hash: transactionID } = useLocation()
  const history = useHistory()

  // redux
  const dispatch = useDispatch()

  // queries
  const { accounts } = useAccountsQuery()
  const selectedAccount = React.useMemo(() => {
    return accounts.find(
      (account) =>
        account.accountId.uniqueKey === addressOrUniqueKey
        || account.address.toLowerCase() === addressOrUniqueKey?.toLowerCase(),
    )
  }, [accounts, addressOrUniqueKey])

  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()
  const { data: networkList = [] } = useGetVisibleNetworksQuery()
  const { userVisibleTokensInfo } = useGetUserTokensRegistryQuery(undefined, {
    selectFromResult: (result) => ({
      userVisibleTokensInfo: selectAllVisibleUserAssetsFromQueryResult(result),
    }),
  })
  const { data: unsortedTransactionList = [] } = useGetTransactionsQuery(
    selectedAccount
      ? {
          accountId: selectedAccount.accountId,
          chainId: null,
          coinType: selectedAccount.accountId.coin,
        }
      : skipToken,
    { skip: !selectedAccount },
  )
  const { data: zcashAccountInfo } = useGetZCashAccountInfoQuery(
    isZCashShieldedTransactionsEnabled
      && selectedAccount?.accountId.coin === BraveWallet.CoinType.ZEC
      ? selectedAccount.accountId
      : skipToken,
  )

  const isShieldedAccount =
    isZCashShieldedTransactionsEnabled
    && !!zcashAccountInfo
    && !!zcashAccountInfo.accountShieldBirthday

  const { data: chainTipStatus } = useGetChainTipStatusQuery(
    isShieldedAccount && selectedAccount
      ? selectedAccount.accountId
      : skipToken,
  )

  const [clearChainTipStatusCache] = useClearChainTipStatusCacheMutation()

  const retryChainTipStatus = React.useCallback(async () => {
    await clearChainTipStatusCache()
  }, [clearChainTipStatusCache])

  useInterval(retryChainTipStatus, 60000, 60000)

  const { data: zcashBalance } = useGetZCashBalanceQuery(
    isShieldedAccount && selectedAccount
      ? {
          chainId: BraveWallet.Z_CASH_MAINNET,
          accountId: selectedAccount.accountId,
        }
      : skipToken,
  )

  // state
  const [showAddNftModal, setShowAddNftModal] = React.useState<boolean>(false)
  const [showViewOnBlockExplorerModal, setShowViewOnBlockExplorerModal] =
    React.useState<boolean>(false)
  const [showSyncAccountModal, setShowSyncAccountModal] =
    React.useState<boolean>(false)
  const [syncWarningDismissed, setSyncWarningDismissed] =
    React.useState<boolean>(false)

  // Computed
  const blocksBehind = chainTipStatus
    ? chainTipStatus.chainTip - chainTipStatus.latestScannedBlock
    : 0

  const isAccountSyncing = useIsAccountSyncing(selectedAccount?.accountId)

  const showSyncWarning = isShieldedAccount && !syncWarningDismissed

  const enableSyncButton =
    !isAccountSyncing && showSyncWarning && blocksBehind > 0

  // custom hooks & memos
  const scrollIntoView = useScrollIntoView()

  const networksFilteredByAccountsCoinType = React.useMemo(() => {
    return !selectedAccount
      ? []
      : networkList.filter(
          (network) => network.coin === selectedAccount.accountId.coin,
        )
  }, [networkList, selectedAccount])

  const networkForSelectedAccount = React.useMemo(() => {
    if (!selectedAccount) {
      return
    }
    // BTC, ZEC and FIL use different accounts for testnet.
    // This checks if the selectedAccount is a testnet account
    // and finds the appropriate network to use.
    if (
      INDIVIDUAL_TESTNET_ACCOUNT_KEYRING_IDS.includes(
        selectedAccount.accountId.keyringId,
      )
    )
      return networksFilteredByAccountsCoinType.find((network) =>
        SupportedTestNetworks.includes(network.chainId),
      )
    return networksFilteredByAccountsCoinType.find(
      (network) => !SupportedTestNetworks.includes(network.chainId),
    )
  }, [selectedAccount, networksFilteredByAccountsCoinType])

  const onClickViewOnBlockExplorer = useExplorer(networkForSelectedAccount)

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
        && network.coin === selectedAccount.accountId.coin,
    )
    const coinName = CoinTypesMap[selectedAccount.accountId.coin]
    const localHostCoins = userVisibleTokensInfo.filter(
      (token) => token.chainId === BraveWallet.LOCALHOST_CHAIN_ID,
    )
    const accountsLocalHost = localHostCoins.find(
      (token) => token.symbol.toUpperCase() === coinName,
    )
    const chainList = filterNetworksForAccount(
      networkList,
      selectedAccount.accountId,
    ).map((network) => network.chainId)
    const list =
      userVisibleTokensInfo.filter(
        (token) =>
          chainList.includes(token?.chainId ?? '')
          && token.chainId !== BraveWallet.LOCALHOST_CHAIN_ID,
      ) ?? []
    if (
      accountsLocalHost
      && hasLocalHostNetwork
      && selectedAccount.accountId.keyringId !== BraveWallet.KeyringId.kFilecoin
    ) {
      return [...list, accountsLocalHost]
    }
    return list
  }, [userVisibleTokensInfo, selectedAccount, networkList])

  const { data: tokenBalancesRegistry, isLoading: isLoadingBalances } =
    useBalancesFetcher(
      selectedAccount && networkList
        ? {
            accounts: [selectedAccount],
            networks: networkList,
          }
        : skipToken,
    )

  const { data: spamTokenBalancesRegistry } = useBalancesFetcher(
    networkList
      ? {
          accounts,
          networks: networkList,
          isSpamRegistry: true,
        }
      : skipToken,
  )

  const nonFungibleTokens = React.useMemo(
    () =>
      accountsTokensList
        .filter(({ isErc721, isNft }) => isErc721 || isNft)
        .filter((token) =>
          new Amount(
            getBalance(
              selectedAccount?.accountId,
              token,
              tokenBalancesRegistry,
            ),
          ).gt(0),
        ),
    [accountsTokensList, selectedAccount, tokenBalancesRegistry],
  )

  const fungibleTokens = React.useMemo(
    () =>
      accountsTokensList
        .filter(
          ({ isErc721, isErc1155, isNft }) => !(isErc721 || isErc1155 || isNft),
        )
        .filter((token) =>
          new Amount(
            getBalance(
              selectedAccount?.accountId,
              token,
              tokenBalancesRegistry,
            ),
          ).gt(0),
        ),
    [accountsTokensList, selectedAccount, tokenBalancesRegistry],
  )

  const tokenPriceIds = React.useMemo(
    () =>
      fungibleTokens
        .filter((token) =>
          new Amount(
            getBalance(
              selectedAccount?.accountId,
              token,
              tokenBalancesRegistry,
            ),
          ).gt(0),
        )
        .map(getPriceIdForToken),
    [fungibleTokens, selectedAccount, tokenBalancesRegistry],
  )

  const { data: spotPriceRegistry, isLoading: isLoadingSpotPrices } =
    useGetTokenSpotPricesQuery(
      tokenPriceIds.length && defaultFiatCurrency
        ? { ids: tokenPriceIds, toCurrency: defaultFiatCurrency }
        : skipToken,
      querySubscriptionOptions60s,
    )

  const filteredRouteOptions =
    selectedAccount && coinSupportsNFTs(selectedAccount.accountId.coin)
      ? AccountDetailsOptions
      : removedNFTsRouteOptions

  const routeOptions = React.useMemo(() => {
    if (!selectedAccount) return []
    return filteredRouteOptions.map((option) => {
      return {
        ...option,
        route: makeAccountRoute(
          selectedAccount,
          option.route as AccountPageTabs,
        ) as WalletRoutes,
      }
    })
  }, [selectedAccount, filteredRouteOptions])

  const fungibleTokensSortedByValue = React.useMemo(() => {
    if (!selectedAccount || isLoadingSpotPrices || isLoadingBalances) {
      return fungibleTokens
    }
    return [...fungibleTokens].sort(function (a, b) {
      const aBalance = getBalance(
        selectedAccount.accountId,
        a,
        tokenBalancesRegistry,
      )

      const bBalance = getBalance(
        selectedAccount.accountId,
        b,
        tokenBalancesRegistry,
      )

      const aFiatBalance = computeFiatAmount({
        spotPriceRegistry,
        value: aBalance,
        token: a,
      })

      const bFiatBalance = computeFiatAmount({
        spotPriceRegistry,
        value: bBalance,
        token: b,
      })

      return bFiatBalance.minus(aFiatBalance).toNumber()
    })
  }, [
    selectedAccount,
    spotPriceRegistry,
    isLoadingSpotPrices,
    fungibleTokens,
    tokenBalancesRegistry,
    isLoadingBalances,
  ])

  // Methods
  const onRemoveAccount = React.useCallback(() => {
    if (selectedAccount) {
      dispatch(
        AccountsTabActions.setAccountToRemove({
          accountId: selectedAccount.accountId,
          name: selectedAccount.name,
        }),
      )
    }
  }, [selectedAccount, dispatch])

  const onClickMenuOption = React.useCallback(
    (option: AccountModalTypes) => {
      if (option === 'remove') {
        onRemoveAccount()
        return
      }
      if (
        option === 'explorer'
        // Only show the Block Explorer modal if there is
        // more than 1 network to show and if CoinType is
        // ETH or SOL.
        && networksFilteredByAccountsCoinType.length > 1
        && (selectedAccount?.accountId.coin === BraveWallet.CoinType.ETH
          || selectedAccount?.accountId.coin === BraveWallet.CoinType.SOL)
      ) {
        setShowViewOnBlockExplorerModal(true)
        return
      }
      if (option === 'explorer' && selectedAccount?.accountId.address) {
        onClickViewOnBlockExplorer(
          'address',
          selectedAccount.accountId.address,
        )()
        return
      }
      dispatch(AccountsTabActions.setShowAccountModal(true))
      dispatch(AccountsTabActions.setAccountModalType(option))
      dispatch(AccountsTabActions.setSelectedAccount(selectedAccount))
    },
    [
      dispatch,
      onRemoveAccount,
      networksFilteredByAccountsCoinType,
      selectedAccount,
      onClickViewOnBlockExplorer,
    ],
  )

  const checkIsTransactionFocused = React.useCallback(
    (id: string): boolean => {
      if (transactionID !== '') {
        return transactionID.replace('#', '') === id
      }
      return false
    },
    [transactionID],
  )

  const handleScrollIntoView = React.useCallback(
    (id: string, ref: HTMLDivElement | null) => {
      if (checkIsTransactionFocused(id)) {
        scrollIntoView(ref)
      }
    },
    [checkIsTransactionFocused, scrollIntoView],
  )

  const onSelectAsset = React.useCallback(
    (asset: BraveWallet.BlockchainToken) => {
      history.push(
        makePortfolioAssetRoute(
          asset.isErc721 || asset.isNft || asset.isErc1155,
          getAssetIdKey(asset),
        ),
      )
    },
    [history],
  )

  const onStartShieldSync = React.useCallback(async () => {
    if (isPanel && selectedAccount) {
      openTab(
        'brave://wallet'
          + makeAccountRoute(selectedAccount, AccountPageTabs.AccountAssetsSub),
      )
      return
    }
    if (selectedAccount) {
      await startShieldSync(selectedAccount.accountId)
      setShowSyncAccountModal(true)
    }
  }, [startShieldSync, selectedAccount, isPanel])

  const onStopAndCloseShieldSync = React.useCallback(async () => {
    if (selectedAccount) {
      await stopShieldSync(selectedAccount.accountId)
      setShowSyncAccountModal(false)
    }
  }, [stopShieldSync, selectedAccount])

  const showAssetDiscoverySkeleton =
    selectedAccount
    && coinSupportsAssets(selectedAccount.accountId.coin)
    && !assetAutoDiscoveryCompleted

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
      noCardPadding={true}
      useCardInPanel={true}
      cardHeader={
        <AccountDetailsHeader
          account={selectedAccount}
          onClickMenuOption={onClickMenuOption}
          tokenBalancesRegistry={tokenBalancesRegistry}
          isAndroid={isAndroid}
          isPanel={isPanel}
        />
      }
    >
      {showSyncWarning && (
        <SyncAlertWrapper
          margin='0px 0px 32px 0px'
          padding='0px 32px'
        >
          <SyncAlert
            type={
              blocksBehind > 7000
                ? 'error'
                : blocksBehind > 3000
                  ? 'warning'
                  : blocksBehind > 1000
                    ? 'info'
                    : 'notice'
            }
          >
            <div slot='title'>
              {!chainTipStatus
                ? getLocale('braveWalletOutOfSyncTitle')
                : getLocale(
                    blocksBehind < 1000
                      ? 'braveWalletBlocksBehind'
                      : 'braveWalletOutOfSyncBlocksBehindTitle',
                  ).replace('$1', blocksBehind.toLocaleString())}
            </div>
            <div>
              {accountsTokensList
                && zcashBalance
                && zcashBalance.shieldedPendingBalance > 0
                && getLocale('braveWalletZCashPendingBalanceTitle').replace(
                  '$1',
                  formatTokenBalanceWithSymbol(
                    (zcashBalance?.shieldedPendingBalance || 0).toString(),
                    accountsTokensList[0].decimals,
                    accountsTokensList[0].symbol,
                  ),
                )}
            </div>
            {getLocale('braveWalletOutOfSyncDescription')}
            <Row
              slot='actions'
              width='unset'
              gap='8px'
            >
              <Button
                size='small'
                isDisabled={!enableSyncButton}
                onClick={onStartShieldSync}
              >
                <Icon
                  name='refresh'
                  slot='icon-before'
                />
                {isAccountSyncing
                  ? getLocale('braveWalletSyncAccountButtonInProgress')
                  : getLocale('braveWalletSyncAccountButton')}
              </Button>
              <Button
                size='small'
                kind='plain-faint'
                onClick={() => setSyncWarningDismissed(true)}
              >
                {getLocale('braveWalletDismissButton')}
              </Button>
            </Row>
          </SyncAlert>
        </SyncAlertWrapper>
      )}
      <ControlsWrapper fullWidth={true}>
        <SegmentedControl navOptions={routeOptions} />
      </ControlsWrapper>

      {selectedTab === AccountPageTabs.AccountAssetsSub && (
        <>
          <AssetsWrapper fullWidth={true}>
            {fungibleTokensSortedByValue.map((asset) => (
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
                        tokenBalancesRegistry,
                      )
                }
                token={asset}
                spotPrice={
                  spotPriceRegistry && !isLoadingSpotPrices
                    ? getTokenPriceAmountFromRegistry(
                        spotPriceRegistry,
                        asset,
                      ).format()
                    : ''
                }
                isAccountDetails={true}
              />
            ))}
            {showAssetDiscoverySkeleton && (
              <PortfolioAssetItemLoadingSkeleton />
            )}
          </AssetsWrapper>
          {fungibleTokensSortedByValue.length === 0
            && !isLoadingBalances
            && !isLoadingSpotPrices && (
              <EmptyStateWrapper>
                <EmptyTokenListState />
              </EmptyStateWrapper>
            )}
        </>
      )}

      {selectedTab === AccountPageTabs.AccountNFTsSub
        && (nonFungibleTokens?.length !== 0 ? (
          <NFTsWrapper fullWidth={true}>
            <NftGrid>
              {nonFungibleTokens?.map((nft: BraveWallet.BlockchainToken) => (
                <NFTGridViewItem
                  key={getAssetIdKey(nft)}
                  token={nft}
                  onSelectAsset={() => onSelectAsset(nft)}
                  isTokenHidden={false}
                  isTokenSpam={false}
                  isWatchOnly={isTokenWatchOnly(
                    nft,
                    accounts,
                    tokenBalancesRegistry,
                    spamTokenBalancesRegistry,
                  )}
                />
              ))}
            </NftGrid>
          </NFTsWrapper>
        ) : (
          <EmptyStateWrapper>
            <NftsEmptyState onImportNft={() => setShowAddNftModal(true)} />
          </EmptyStateWrapper>
        ))}

      {selectedTab === AccountPageTabs.AccountTransactionsSub && (
        <>
          {transactionList.length !== 0 ? (
            <TransactionsWrapper
              fullWidth={true}
              gap='16px'
            >
              {transactionList.map((transaction) => (
                <PortfolioTransactionItem
                  key={transaction?.id}
                  transaction={transaction}
                  ref={(ref) => handleScrollIntoView(transaction.id, ref)}
                  isFocused={checkIsTransactionFocused(transaction.id)}
                />
              ))}
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

      {showViewOnBlockExplorerModal && (
        <ViewOnBlockExplorerModal
          account={selectedAccount}
          onClose={() => setShowViewOnBlockExplorerModal(false)}
        />
      )}

      {showAddNftModal && (
        <AddOrEditNftModal
          onClose={() => setShowAddNftModal(false)}
          onHideForm={() => setShowAddNftModal(false)}
        />
      )}

      {showSyncAccountModal && (
        <ZCashSyncModal
          account={selectedAccount}
          onClose={onStopAndCloseShieldSync}
        />
      )}
    </WalletPageWrapper>
  )
}

export default Account
