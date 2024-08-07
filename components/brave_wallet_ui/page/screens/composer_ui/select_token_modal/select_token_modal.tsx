// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Selectors
import {
  selectAllVisibleFungibleUserAssetsFromQueryResult,
  selectAllVisibleUserNFTsFromQueryResult
} from '../../../../common/slices/entities/blockchain-token.entity'
import {
  selectAllAccountInfosFromQuery //
} from '../../../../common/slices/entities/account-info.entity'
import { useSafeUISelector } from '../../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../../common/selectors'

// Types
import { BraveWallet, SendPageTabHashes } from '../../../../constants/types'
import {
  TokenBalancesRegistry //
} from '../../../../common/slices/entities/token-balance.entity'

// Utils
import { getLocale } from '../../../../../common/locale'
import {
  filterNetworksForAccount,
  networkSupportsAccount
} from '../../../../utils/network-utils'
import Amount from '../../../../utils/amount'
import { getBalance } from '../../../../utils/balance-utils'
import {
  computeFiatAmount,
  getTokenPriceFromRegistry,
  getPriceIdForToken
} from '../../../../utils/pricing-utils'
import { getAssetIdKey } from '../../../../utils/asset-utils'
import {
  getEntitiesListFromEntityState //
} from '../../../../utils/entities.utils'
import {
  getAccountsForNetwork //
} from '../../../../utils/account-utils'

// Queries
import {
  useGetDefaultFiatCurrencyQuery,
  useGetVisibleNetworksQuery,
  useGetTokenSpotPricesQuery,
  useGetUserTokensRegistryQuery,
  useGetAccountInfosRegistryQuery,
  useGetSwapSupportedNetworksQuery
} from '../../../../common/slices/api.slice'
import {
  useGetCombinedTokensRegistryQuery //
} from '../../../../common/slices/api.slice.extra'
import {
  querySubscriptionOptions60s //
} from '../../../../common/slices/constants'

// hooks
import {
  useBalancesFetcher //
} from '../../../../common/hooks/use-balances-fetcher'

// Options
import {
  AllNetworksOption //
} from '../../../../options/network-filter-options'
import {
  AllAccountsOption //
} from '../../../../options/account-filter-options'

// Components
import {
  TokenListItemSkeleton //
} from '../token_list_item/token_list_item_skeleton'
import {
  PopupModal //
} from '../../../../components/desktop/popup-modals/index'
import {
  SelectSendOptionButtons //
} from '../select_send_option_buttons/select_send_option_buttons'
import {
  VirtualizedTokenList //
} from '../virtualized_token_list/virtualized_tokens_list'
import { SelectAccount } from './select_account/select_account'
import { TokenDetails } from './token_details/token_details'
import { NetworksDropdown } from '../../../../components/shared/dropdowns/networks_dropdown'
import { AccountsDropdown } from '../../../../components/shared/dropdowns/accounts_dropdown'
import {
  BottomSheet //
} from '../../../../components/shared/bottom_sheet/bottom_sheet'

// Styled Components
import { Row, Column } from '../../../../components/shared/style'
import {
  ScrollContainer,
  NoAssetsText,
  SearchInput
} from './select_token_modal.style'

const checkIsSwapDropdownOptionDisabled = (
  account: BraveWallet.AccountInfo,
  network: BraveWallet.NetworkInfo
) => {
  if (
    account.accountId.uniqueKey === AllAccountsOption.accountId.uniqueKey ||
    network.chainId === AllNetworksOption.chainId
  ) {
    return false
  }
  return account.accountId.coin !== network.coin
}

const checkIsBridgeNetworkDropdownOptionDisabled = (
  networkChainId: string,
  tokenChainId: string
) => {
  return networkChainId === tokenChainId
}

const getFullAssetBalance = (
  asset: BraveWallet.BlockchainToken,
  networks: BraveWallet.NetworkInfo[],
  accounts: BraveWallet.AccountInfo[],
  tokenBalancesRegistry?: TokenBalancesRegistry | null
) => {
  if (!tokenBalancesRegistry) {
    return ''
  }

  const network = networks.find(
    (network) =>
      network.coin === asset.coin && network.chainId === asset.chainId
  )

  if (!network) {
    return '0'
  }

  const amounts = accounts
    .filter((account) => {
      return network && networkSupportsAccount(network, account.accountId)
    })
    .map((account) =>
      getBalance(account.accountId, asset, tokenBalancesRegistry)
    )

  if (amounts.length === 0) {
    return '0'
  }

  return amounts.reduce(function (a, b) {
    return a !== '' && b !== '' ? new Amount(a).plus(b).format() : ''
  }, '0')
}

interface Props {
  onClose: () => void
  selectedSendOption: SendPageTabHashes
  selectedFromToken?: BraveWallet.BlockchainToken
  selectedToToken?: BraveWallet.BlockchainToken
  selectingFromOrTo: 'from' | 'to'
  onSelectAsset: (
    asset: BraveWallet.BlockchainToken,
    account?: BraveWallet.AccountInfo
  ) => void
  onSelectSendOption?: (sendOption: SendPageTabHashes) => void
  selectedNetwork?: BraveWallet.NetworkInfo
  modalType: 'send' | 'swap' | 'bridge'
}

export const SelectTokenModal = React.forwardRef<HTMLDivElement, Props>(
  (props: Props, forwardedRef) => {
    const {
      onClose,
      selectedSendOption,
      selectedFromToken,
      selectedToToken,
      selectingFromOrTo,
      onSelectAsset,
      onSelectSendOption,
      selectedNetwork,
      modalType
    } = props

    // State
    const [searchValue, setSearchValue] = React.useState<string>('')
    const [selectedNetworkFilter, setSelectedNetworkFilter] =
      React.useState<BraveWallet.NetworkInfo>(
        selectedNetwork || AllNetworksOption
      )
    const [selectedAccountFilter, setSelectedAccountFilter] =
      React.useState<BraveWallet.AccountInfo>(AllAccountsOption)
    const [pendingSelectedAsset, setPendingSelectedAsset] = React.useState<
      BraveWallet.BlockchainToken | undefined
    >(undefined)
    const [tokenDetails, setTokenDetails] = React.useState<
      BraveWallet.BlockchainToken | undefined
    >(undefined)

    // Selectors
    const isPanel = useSafeUISelector(UISelectors.isPanel)

    // Queries & Mutations
    const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()
    const { data: visibleNetworks = [] } = useGetVisibleNetworksQuery(
      undefined,
      {
        skip: modalType !== 'send'
      }
    )
    const { data: swapNetworks = [] } = useGetSwapSupportedNetworksQuery(
      undefined,
      {
        skip: modalType === 'send'
      }
    )

    const { accounts } = useGetAccountInfosRegistryQuery(undefined, {
      selectFromResult: (res) => ({
        accounts: selectAllAccountInfosFromQuery(res)
      })
    })

    const { data: combinedTokenRegistry } = useGetCombinedTokensRegistryQuery()

    const fullVisibleFungibleTokensList = React.useMemo(() => {
      if (!combinedTokenRegistry) {
        return []
      }
      return getEntitiesListFromEntityState(
        combinedTokenRegistry,
        combinedTokenRegistry.fungibleVisibleTokenIds
      )
    }, [combinedTokenRegistry])

    const { userVisibleFungibleTokens, userVisibleNfts } =
      useGetUserTokensRegistryQuery(undefined, {
        selectFromResult: (result) => ({
          userVisibleFungibleTokens:
            selectAllVisibleFungibleUserAssetsFromQueryResult(result),
          userVisibleNfts: selectAllVisibleUserNFTsFromQueryResult(result)
        })
      })

    const bridgeAndSwapNetworks = React.useMemo(() => {
      if (
        modalType === 'bridge' &&
        selectingFromOrTo === 'to' &&
        selectedFromToken
      ) {
        return swapNetworks.filter(
          (network) =>
            !checkIsBridgeNetworkDropdownOptionDisabled(
              network.chainId,
              selectedFromToken.chainId
            )
        )
      }
      if (
        modalType === 'bridge' &&
        selectingFromOrTo === 'from' &&
        selectedToToken
      ) {
        return swapNetworks.filter(
          (network) =>
            !checkIsBridgeNetworkDropdownOptionDisabled(
              network.chainId,
              selectedToToken.chainId
            )
        )
      }
      return swapNetworks
    }, [
      modalType,
      selectingFromOrTo,
      selectedFromToken,
      selectedToToken,
      swapNetworks
    ])

    const networks =
      modalType === 'send' ? visibleNetworks : bridgeAndSwapNetworks

    const { data: tokenBalancesRegistry, isLoading: isLoadingBalances } =
      useBalancesFetcher({
        accounts,
        networks
      })

    // Methods
    const getAllAccountsWithBalance = React.useCallback(
      (asset: BraveWallet.BlockchainToken) => {
        if (!tokenBalancesRegistry) {
          return []
        }

        return accounts.filter((account) =>
          new Amount(
            getBalance(account.accountId, asset, tokenBalancesRegistry)
          ).gt(0)
        )
      },
      [tokenBalancesRegistry, accounts]
    )

    const tokensBySelectedComposerOption = React.useMemo(() => {
      if (modalType === 'swap' || modalType === 'bridge') {
        return fullVisibleFungibleTokensList.filter((token) =>
          bridgeAndSwapNetworks.some(({ chainId }) => chainId === token.chainId)
        )
      }

      if (selectedSendOption === SendPageTabHashes.nft) {
        return userVisibleNfts.filter(
          (token) => getAllAccountsWithBalance(token).length > 0
        )
      }

      return userVisibleFungibleTokens.filter(
        (token) => getAllAccountsWithBalance(token).length > 0
      )
    }, [
      modalType,
      fullVisibleFungibleTokensList,
      bridgeAndSwapNetworks,
      userVisibleNfts,
      userVisibleFungibleTokens,
      selectedSendOption,
      getAllAccountsWithBalance
    ])

    // Memos
    const userTokenBalances: Record<string, string> = React.useMemo(() => {
      if (!tokenBalancesRegistry) {
        // wait for balances before computing this list
        return {}
      }
      const balancesMap: Record<string, string> = {}
      for (const asset of tokensBySelectedComposerOption) {
        balancesMap[getAssetIdKey(asset)] =
          selectedAccountFilter.accountId.uniqueKey ===
          AllAccountsOption.accountId.uniqueKey
            ? getFullAssetBalance(
                asset,
                networks,
                accounts,
                tokenBalancesRegistry
              )
            : getBalance(
                selectedAccountFilter.accountId,
                asset,
                tokenBalancesRegistry
              )
      }
      return balancesMap
    }, [
      tokensBySelectedComposerOption,
      accounts,
      networks,
      tokenBalancesRegistry,
      selectedAccountFilter
    ])

    const tokenPriceIds = React.useMemo(
      () =>
        tokensBySelectedComposerOption
          .filter(
            (token) => !token.isNft && !token.isErc721 && !token.isErc1155
          )
          .filter((token) =>
            new Amount(userTokenBalances[getAssetIdKey(token)]).gt(0)
          )
          .map((token) => getPriceIdForToken(token)),
      [tokensBySelectedComposerOption, userTokenBalances]
    )

    const { data: spotPriceRegistry, isLoading: isLoadingSpotPrices } =
      useGetTokenSpotPricesQuery(
        !isLoadingBalances &&
          tokenPriceIds.length &&
          defaultFiatCurrency &&
          selectedSendOption !== '#nft'
          ? {
              ids: tokenPriceIds,
              timeframe: BraveWallet.AssetPriceTimeframe.OneDay,
              toCurrency: defaultFiatCurrency
            }
          : skipToken,
        querySubscriptionOptions60s
      )

    const tokensSortedByValue = React.useMemo(() => {
      return [...tokensBySelectedComposerOption].sort(function (a, b) {
        const aBalance = userTokenBalances[getAssetIdKey(a)]
        const bBalance = userTokenBalances[getAssetIdKey(b)]

        const bFiatBalance = computeFiatAmount({
          spotPriceRegistry,
          value: bBalance,
          token: b
        })

        const aFiatBalance = computeFiatAmount({
          spotPriceRegistry,
          value: aBalance,
          token: a
        })

        return bFiatBalance.minus(aFiatBalance).toNumber()
      })
    }, [tokensBySelectedComposerOption, userTokenBalances, spotPriceRegistry])

    const tokensFilteredByNetwork = React.useMemo(() => {
      if (selectedNetworkFilter.chainId === AllNetworksOption.chainId) {
        return tokensSortedByValue
      }
      return tokensSortedByValue.filter(
        (token) =>
          token.chainId === selectedNetworkFilter.chainId &&
          token.coin === selectedNetworkFilter.coin
      )
    }, [tokensSortedByValue, selectedNetworkFilter])

    const tokensFilteredByAccount = React.useMemo(() => {
      if (
        selectedAccountFilter.accountId.uniqueKey ===
        AllAccountsOption.accountId.uniqueKey
      ) {
        return tokensFilteredByNetwork
      }

      const chainList = filterNetworksForAccount(
        networks,
        selectedAccountFilter.accountId
      ).map((network) => network.chainId)

      return tokensFilteredByNetwork.filter(
        (token) =>
          token.coin === selectedAccountFilter.accountId.coin &&
          chainList.includes(token.chainId)
      )
    }, [networks, selectedAccountFilter, tokensFilteredByNetwork])

    const tokensBySearchValue = React.useMemo(() => {
      if (searchValue === '') {
        return tokensFilteredByAccount
      }
      const lowercaseSearchValue = searchValue.toLowerCase()
      return tokensFilteredByAccount.filter(
        (token) =>
          token.name.toLowerCase() === lowercaseSearchValue ||
          token.name.toLowerCase().includes(lowercaseSearchValue) ||
          token.symbol.toLocaleLowerCase() === lowercaseSearchValue ||
          token.symbol.toLowerCase().includes(lowercaseSearchValue) ||
          token.contractAddress.toLocaleLowerCase() === lowercaseSearchValue
      )
    }, [tokensFilteredByAccount, searchValue])

    const firstNoBalanceTokenKey = React.useMemo(() => {
      const token = tokensBySearchValue.find((token) =>
        new Amount(userTokenBalances[getAssetIdKey(token)]).isZero()
      )
      return token ? getAssetIdKey(token) : ''
    }, [tokensBySearchValue, userTokenBalances])

    const accountsForPendingSelectedAsset = React.useMemo(() => {
      if (!pendingSelectedAsset) {
        return []
      }
      return getAccountsForNetwork(pendingSelectedAsset, accounts).sort(
        function (a, b) {
          return new Amount(
            getBalance(b.accountId, pendingSelectedAsset, tokenBalancesRegistry)
          )
            .minus(
              getBalance(
                a.accountId,
                pendingSelectedAsset,
                tokenBalancesRegistry
              )
            )
            .toNumber()
        }
      )
    }, [accounts, pendingSelectedAsset, tokenBalancesRegistry])

    // Methods
    const handleSelectAsset = React.useCallback(
      (token: BraveWallet.BlockchainToken) => {
        // No need to select an account when selecting
        // a token to receive right now.
        // This will change with bridge.
        if (selectingFromOrTo === 'to' && modalType === 'swap') {
          onSelectAsset(token)
          onClose()
          return
        }

        setPendingSelectedAsset(token)
      },
      [onSelectAsset, onClose, selectingFromOrTo, modalType]
    )

    const handleSelectAccount = React.useCallback(
      (account: BraveWallet.AccountInfo) => {
        if (pendingSelectedAsset) {
          onSelectAsset(pendingSelectedAsset, account)
          onClose()
        }
      },
      [onSelectAsset, onClose, pendingSelectedAsset]
    )

    const checkIsNetworkOptionDisabled = React.useCallback(
      (network: BraveWallet.NetworkInfo) => {
        if (modalType === 'swap') {
          return checkIsSwapDropdownOptionDisabled(
            selectedAccountFilter,
            network
          )
        }
        if (
          modalType === 'bridge' &&
          selectingFromOrTo === 'to' &&
          selectedFromToken
        ) {
          return checkIsBridgeNetworkDropdownOptionDisabled(
            selectedFromToken.chainId,
            network.chainId
          )
        }
        if (
          modalType === 'bridge' &&
          selectingFromOrTo === 'from' &&
          selectedToToken
        ) {
          return checkIsBridgeNetworkDropdownOptionDisabled(
            selectedToToken.chainId,
            network.chainId
          )
        }
        return false
      },
      [
        modalType,
        selectingFromOrTo,
        selectedAccountFilter,
        selectedFromToken,
        selectedToToken
      ]
    )

    const checkIsAccountOptionDisabled = React.useCallback(
      (account: BraveWallet.AccountInfo) => {
        if (modalType === 'swap') {
          return checkIsSwapDropdownOptionDisabled(
            account,
            selectedNetworkFilter
          )
        }
        if (
          modalType === 'bridge' &&
          selectingFromOrTo === 'to' &&
          selectedFromToken
        ) {
          return (
            account.accountId.coin === BraveWallet.CoinType.SOL &&
            selectedFromToken.coin === BraveWallet.CoinType.SOL
          )
        }
        if (
          modalType === 'bridge' &&
          selectingFromOrTo === 'from' &&
          selectedToToken
        ) {
          return (
            account.accountId.coin === BraveWallet.CoinType.SOL &&
            selectedToToken.coin === BraveWallet.CoinType.SOL
          )
        }
        return false
      },
      [
        modalType,
        selectingFromOrTo,
        selectedFromToken,
        selectedToToken,
        selectedNetworkFilter
      ]
    )

    const onSelectNetworkFilter = React.useCallback(
      (network: BraveWallet.NetworkInfo) => {
        if (checkIsNetworkOptionDisabled(network)) {
          return
        }
        setSelectedNetworkFilter(network)
      },
      [checkIsNetworkOptionDisabled]
    )

    const onSelectAccountFilter = React.useCallback(
      (uniqueKey: string) => {
        const account =
          accounts.find((a) => a.accountId.uniqueKey === uniqueKey) ??
          AllAccountsOption
        if (checkIsAccountOptionDisabled(account)) {
          return
        }
        setSelectedAccountFilter(account)
      },
      [accounts, checkIsAccountOptionDisabled]
    )

    // Computed & Memos
    const emptyTokensList =
      !isLoadingBalances && tokensBySearchValue.length === 0

    const tokenList = React.useMemo(() => {
      if (isLoadingBalances || isLoadingSpotPrices) {
        return (
          <TokenListItemSkeleton
            isNFT={selectedSendOption === SendPageTabHashes.nft}
          />
        )
      }
      if (emptyTokensList) {
        return (
          <NoAssetsText
            textSize='14px'
            isBold={false}
          >
            {getLocale('braveWalletNoAvailableTokens')}
          </NoAssetsText>
        )
      }

      return (
        <VirtualizedTokenList
          onSelectToken={handleSelectAsset}
          tokenList={tokensBySearchValue}
          selectedFromToken={selectedFromToken}
          selectedToToken={selectedToToken}
          selectingFromOrTo={selectingFromOrTo}
          spotPriceRegistry={spotPriceRegistry}
          isLoadingSpotPrices={isLoadingSpotPrices}
          getAllAccountsWithBalance={getAllAccountsWithBalance}
          firstNoBalanceTokenKey={firstNoBalanceTokenKey}
          onViewTokenDetails={setTokenDetails}
          modalType={modalType}
          userTokenBalances={userTokenBalances}
        />
      )
    }, [
      isLoadingBalances,
      emptyTokensList,
      selectedSendOption,
      handleSelectAsset,
      selectedFromToken,
      selectedToToken,
      selectingFromOrTo,
      tokensBySearchValue,
      spotPriceRegistry,
      isLoadingSpotPrices,
      firstNoBalanceTokenKey,
      modalType,
      getAllAccountsWithBalance,
      userTokenBalances
    ])

    const swapSupportedAccounts = React.useMemo(() => {
      return accounts.filter(
        (account) =>
          account.accountId.coin === BraveWallet.CoinType.ETH ||
          account.accountId.coin === BraveWallet.CoinType.SOL
      )
    }, [accounts])

    // render

    if (!isPanel && pendingSelectedAsset) {
      return (
        <PopupModal
          title=''
          onClose={onClose}
          onBack={() => setPendingSelectedAsset(undefined)}
          width='560px'
          showDivider={false}
        >
          <SelectAccount
            token={pendingSelectedAsset}
            accounts={accountsForPendingSelectedAsset}
            tokenBalancesRegistry={tokenBalancesRegistry}
            spotPrice={
              spotPriceRegistry
                ? getTokenPriceFromRegistry(
                    spotPriceRegistry,
                    pendingSelectedAsset
                  )
                : undefined
            }
            onSelectAccount={handleSelectAccount}
          />
        </PopupModal>
      )
    }

    if (!isPanel && tokenDetails) {
      return (
        <PopupModal
          title=''
          onClose={onClose}
          onBack={() => setTokenDetails(undefined)}
          width='560px'
          showDivider={false}
        >
          <TokenDetails token={tokenDetails} />
        </PopupModal>
      )
    }

    return (
      <>
        <PopupModal
          onClose={onClose}
          title={getLocale(
            modalType === 'swap'
              ? 'braveWalletChooseAssetToSwap'
              : modalType === 'bridge'
              ? 'braveWalletChooseAssetToBridge'
              : 'braveWalletChooseAssetToSend'
          )}
          width='560px'
          height='90vh'
          ref={isPanel ? undefined : forwardedRef}
        >
          {onSelectSendOption && selectedSendOption && (
            <Row
              justifyContent='flex-start'
              padding='0px 16px'
            >
              <SelectSendOptionButtons
                onClick={onSelectSendOption}
                selectedSendOption={selectedSendOption}
              />
            </Row>
          )}
          <Column
            fullWidth={true}
            padding='16px'
          >
            <SearchInput
              value={searchValue}
              onInput={(e) => setSearchValue(e.value)}
              placeholder={
                selectedSendOption === SendPageTabHashes.nft
                  ? getLocale('braveWalletSearchNFTs')
                  : getLocale('braveWalletSearchTokens')
              }
              type='text'
            >
              <div slot='left-icon'>
                <Icon name='search' />
              </div>
            </SearchInput>
            {!(selectingFromOrTo === 'to' && modalType === 'swap') && (
              <Row
                padding='16px 0px 0px 0px'
                gap='8px'
              >
                <AccountsDropdown
                  accounts={
                    modalType === 'send' ? accounts : swapSupportedAccounts
                  }
                  selectedAccount={selectedAccountFilter}
                  showAllAccountsOption={true}
                  onSelectAccount={onSelectAccountFilter}
                  checkIsAccountOptionDisabled={checkIsAccountOptionDisabled}
                />
                <NetworksDropdown
                  networks={
                    modalType === 'send' ? visibleNetworks : swapNetworks
                  }
                  selectedNetwork={selectedNetworkFilter}
                  showAllNetworksOption={true}
                  onSelectNetwork={onSelectNetworkFilter}
                  checkIsNetworkOptionDisabled={checkIsNetworkOptionDisabled}
                />
              </Row>
            )}
          </Column>
          <ScrollContainer
            fullWidth={true}
            justifyContent={emptyTokensList ? 'center' : 'flex-start'}
          >
            {tokenList}
          </ScrollContainer>
        </PopupModal>
        {isPanel && (
          <BottomSheet
            onClose={() => setPendingSelectedAsset(undefined)}
            isOpen={pendingSelectedAsset !== undefined}
          >
            {pendingSelectedAsset && (
              <SelectAccount
                token={pendingSelectedAsset}
                accounts={accountsForPendingSelectedAsset}
                tokenBalancesRegistry={tokenBalancesRegistry}
                spotPrice={
                  spotPriceRegistry
                    ? getTokenPriceFromRegistry(
                        spotPriceRegistry,
                        pendingSelectedAsset
                      )
                    : undefined
                }
                onSelectAccount={handleSelectAccount}
              />
            )}
          </BottomSheet>
        )}
        {isPanel && (
          <BottomSheet
            onClose={() => setTokenDetails(undefined)}
            isOpen={tokenDetails !== undefined}
          >
            {tokenDetails && <TokenDetails token={tokenDetails} />}
          </BottomSheet>
        )}
      </>
    )
  }
)
