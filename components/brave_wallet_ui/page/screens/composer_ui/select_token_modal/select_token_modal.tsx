// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Selectors
import {
  selectAllVisibleUserAssetsFromQueryResult //
} from '../../../../common/slices/entities/blockchain-token.entity'
import {
  selectAllAccountInfosFromQuery //
} from '../../../../common/slices/entities/account-info.entity'

// Types
import {
  BraveWallet,
  CoinTypesMap,
  SendPageTabHashes
} from '../../../../constants/types'

// Utils
import { getLocale } from '../../../../../common/locale'
import { getPriceIdForToken } from '../../../../utils/api-utils'
import { filterNetworksForAccount } from '../../../../utils/network-utils'
import {
  computeFiatAmount,
  getTokenPriceAmountFromRegistry
} from '../../../../utils/pricing-utils'
import Amount from '../../../../utils/amount'
import { getBalance } from '../../../../utils/balance-utils'
import { getAssetIdKey } from '../../../../utils/asset-utils'
import { reduceAddress } from '../../../../utils/reduce-address'

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
  useGetCombinedTokensListQuery //
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

// Components
import { TokenListItem } from '../token_list_item/token_list_item'
import {
  TokenListItemSkeleton //
} from '../token_list_item/token_list_item_skeleton'
import {
  LoadingSkeleton //
} from '../../../../components/shared/loading-skeleton/index'
import {
  NetworkFilterWithSearch //
} from '../../../../components/desktop/network-filter-with-search'
import {
  PopupModal //
} from '../../../../components/desktop/popup-modals/index'
import {
  SelectSendOptionButtons //
} from '../select_send_option_buttons/select_send_option_buttons'
import {
  VirtualizedTokenList //
} from '../virtualized_token_list/virtualized_tokens_list'

// Styled Components
import { Row, Column } from '../../../../components/shared/style'
import {
  ScrollContainer,
  AccountSection,
  AccountAddressText,
  AccountNameText,
  BalanceText,
  SearchBarRow,
  NoAssetsText,
  SendOptionsRow
} from './select_token_modal.style'

const getIsTokenSelected = (
  token: BraveWallet.BlockchainToken,
  selectedToken?: BraveWallet.BlockchainToken
) => {
  if (!selectedToken) {
    return false
  }
  return (
    selectedToken.contractAddress === token.contractAddress &&
    selectedToken.coin === token.coin &&
    selectedToken.chainId === token.chainId
  )
}

interface Props {
  onClose: () => void
  selectedSendOption: SendPageTabHashes
  selectedFromToken?: BraveWallet.BlockchainToken
  selectedToToken?: BraveWallet.BlockchainToken
  onSelectAsset: (
    asset: BraveWallet.BlockchainToken,
    account?: BraveWallet.AccountInfo
  ) => void
  onSelectSendOption?: (sendOption: SendPageTabHashes) => void
  selectedNetwork?: BraveWallet.NetworkInfo
  showFullFlatTokenList?: boolean
  modalType: 'send' | 'swap' | 'bridge'
}

export const SelectTokenModal = React.forwardRef<HTMLDivElement, Props>(
  (props: Props, forwardedRef) => {
    const {
      onClose,
      selectedSendOption,
      selectedFromToken,
      selectedToToken,
      onSelectAsset,
      onSelectSendOption,
      selectedNetwork,
      showFullFlatTokenList,
      modalType
    } = props

    // State
    const [searchValue, setSearchValue] = React.useState<string>('')
    const [showNetworkDropDown, setShowNetworkDropDown] =
      React.useState<boolean>(false)
    const [selectedNetworkFilter, setSelectedNetworkFilter] =
      React.useState<BraveWallet.NetworkInfo>(
        selectedNetwork || AllNetworksOption
      )

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
        skip: modalType !== 'swap'
      }
    )

    const { accounts } = useGetAccountInfosRegistryQuery(undefined, {
      selectFromResult: (res) => ({
        accounts: selectAllAccountInfosFromQuery(res)
      })
    })

    const { userVisibleTokensInfo } = useGetUserTokensRegistryQuery(undefined, {
      selectFromResult: (result) => ({
        userVisibleTokensInfo: selectAllVisibleUserAssetsFromQueryResult(result)
      })
    })

    const { data: fullTokenList } = useGetCombinedTokensListQuery(
      showFullFlatTokenList ? undefined : skipToken
    )

    const networks = modalType === 'swap' ? swapNetworks : visibleNetworks

    const { data: tokenBalancesRegistry, isLoading: isLoadingBalances } =
      useBalancesFetcher(
        !showFullFlatTokenList
          ? {
              accounts,
              networks
            }
          : skipToken
      )

    // Methods
    const getTokenListByAccount = React.useCallback(
      (account: BraveWallet.AccountInfo) => {
        if (!account || !networks) {
          return []
        }
        // Since LOCALHOST's chainId is shared between coinType's
        // this check will make sure we are returning the correct
        // LOCALHOST asset for each account.
        const coinName = CoinTypesMap[account.accountId.coin]
        const localHostCoins = userVisibleTokensInfo.filter(
          (token) => token.chainId === BraveWallet.LOCALHOST_CHAIN_ID
        )
        const accountsLocalHost = localHostCoins.find(
          (token) => token.symbol.toUpperCase() === coinName
        )

        const chainList = filterNetworksForAccount(
          networks,
          account.accountId
        ).map((network) => network.chainId)

        const list = userVisibleTokensInfo.filter(
          (token) =>
            token.chainId !== BraveWallet.LOCALHOST_CHAIN_ID &&
            chainList.includes(token?.chainId ?? '')
        )

        if (
          accountsLocalHost &&
          account.accountId.keyringId !== BraveWallet.KeyringId.kFilecoin
        ) {
          list.push(accountsLocalHost)
          return list
        }

        return list.filter((token) => token.visible)
      },
      [userVisibleTokensInfo, networks]
    )

    const getTokenListWithBalances = React.useCallback(
      (account: BraveWallet.AccountInfo) => {
        return getTokenListByAccount(account).filter((token) =>
          new Amount(
            getBalance(account.accountId, token, tokenBalancesRegistry)
          ).gt(0)
        )
      },
      [getTokenListByAccount, tokenBalancesRegistry]
    )

    const getTokensBySelectedSendOption = React.useCallback(
      (account: BraveWallet.AccountInfo) => {
        if (selectedSendOption === SendPageTabHashes.nft) {
          return getTokenListWithBalances(account).filter(
            (token) => token.isErc721 || token.isNft || token.isErc1155
          )
        }
        return getTokenListWithBalances(account).filter(
          (token) => !token.isErc721 && !token.isErc1155 && !token.isNft
        )
      },
      [getTokenListWithBalances, selectedSendOption]
    )

    const tokenPriceIds = React.useMemo(
      () =>
        accounts
          .flatMap(getTokensBySelectedSendOption)
          .filter(
            (token) => !token.isErc721 && !token.isErc1155 && !token.isNft
          )
          .map(getPriceIdForToken),
      [accounts, getTokensBySelectedSendOption]
    )
    const { data: spotPriceRegistry, isLoading: isLoadingSpotPrices } =
      useGetTokenSpotPricesQuery(
        !isLoadingBalances &&
          tokenPriceIds.length &&
          defaultFiatCurrency &&
          !showFullFlatTokenList
          ? { ids: tokenPriceIds, toCurrency: defaultFiatCurrency }
          : skipToken,
        querySubscriptionOptions60s
      )

    const getTokensByNetwork = React.useCallback(
      (account?: BraveWallet.AccountInfo) => {
        if (!selectedNetworkFilter) {
          return []
        }
        if (!account || showFullFlatTokenList) {
          if (selectedNetworkFilter.chainId === AllNetworksOption.chainId) {
            return fullTokenList.filter(
              (token) => !token.isNft && !token.isErc1155 && !token.isErc721
            )
          }
          return fullTokenList.filter(
            (token) =>
              token.chainId === selectedNetworkFilter.chainId &&
              token.coin === selectedNetworkFilter.coin &&
              !token.isNft &&
              !token.isErc1155 &&
              !token.isErc721
          )
        }
        if (selectedNetworkFilter.chainId === AllNetworksOption.chainId) {
          return getTokensBySelectedSendOption(account)
        }
        return getTokensBySelectedSendOption(account).filter(
          (token) =>
            token.chainId === selectedNetworkFilter.chainId &&
            token.coin === selectedNetworkFilter.coin
        )
      },
      [
        getTokensBySelectedSendOption,
        selectedNetworkFilter,
        fullTokenList,
        showFullFlatTokenList
      ]
    )

    const getTokensBySearchValue = React.useCallback(
      (account?: BraveWallet.AccountInfo) => {
        if (searchValue === '') {
          return getTokensByNetwork(account)
        }
        return getTokensByNetwork(account).filter(
          (token) =>
            token.name.toLowerCase() === searchValue.toLowerCase() ||
            token.name.toLowerCase().startsWith(searchValue.toLowerCase()) ||
            token.symbol.toLocaleLowerCase() === searchValue.toLowerCase() ||
            token.symbol.toLowerCase().startsWith(searchValue.toLowerCase()) ||
            token.contractAddress.toLocaleLowerCase() ===
              searchValue.toLowerCase()
        )
      },
      [getTokensByNetwork, searchValue]
    )

    const getAccountFiatValue = React.useCallback(
      (account: BraveWallet.AccountInfo) => {
        const amounts = getTokensBySearchValue(account).map((token) => {
          const balance = getBalance(
            account.accountId,
            token,
            tokenBalancesRegistry
          )

          return computeFiatAmount({
            spotPriceRegistry,
            value: balance,
            token
          }).format()
        })

        const reducedAmounts = amounts.reduce(function (a, b) {
          return a !== '' && b !== '' ? new Amount(a).plus(b).format() : ''
        })
        return new Amount(reducedAmounts).formatAsFiat(defaultFiatCurrency)
      },
      [
        getTokensBySearchValue,
        spotPriceRegistry,
        defaultFiatCurrency,
        tokenBalancesRegistry
      ]
    )

    const handleSelectAsset = React.useCallback(
      (
        token: BraveWallet.BlockchainToken,
        account?: BraveWallet.AccountInfo
      ) => {
        onSelectAsset(token, account)
        onClose()
      },
      [onSelectAsset, onClose]
    )

    const onSearch = React.useCallback(
      (event: React.ChangeEvent<HTMLInputElement>) => {
        setSearchValue(event.target.value)
      },
      []
    )

    const onToggleShowNetworkDropdown = React.useCallback(() => {
      setShowNetworkDropDown((prev) => !prev)
    }, [])

    const onSelectAssetsNetwork = React.useCallback(
      (network: BraveWallet.NetworkInfo) => {
        setSelectedNetworkFilter(network)
        setShowNetworkDropDown(false)
      },
      []
    )

    // Memos
    const emptyTokensList = React.useMemo(() => {
      return (
        !isLoadingBalances &&
        accounts.map((account) => getTokensBySearchValue(account)).flat(1)
          .length === 0
      )
    }, [accounts, getTokensBySearchValue, isLoadingBalances])

    const tokensByAccount = React.useMemo(() => {
      if (isLoadingBalances && !showFullFlatTokenList) {
        return (
          <Column fullWidth={true}>
            <AccountSection
              width='100%'
              padding='0px 8px 12px 8px'
              marginBottom={11}
              justifyContent='space-between'
            >
              <Column>
                <LoadingSkeleton
                  width={80}
                  height={14}
                />
              </Column>
              <Column>
                <LoadingSkeleton
                  width={80}
                  height={14}
                />
              </Column>
            </AccountSection>
            <Column
              fullWidth={true}
              padding='0px 0px 8px 0px'
            >
              <TokenListItemSkeleton
                isNFT={selectedSendOption === SendPageTabHashes.nft}
              />
            </Column>
          </Column>
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

      if (showFullFlatTokenList) {
        return (
          <VirtualizedTokenList
            onSelectToken={handleSelectAsset}
            tokenList={getTokensBySearchValue()}
            selectedToToken={selectedToToken}
            selectedFromToken={selectedFromToken}
          />
        )
      }

      return accounts.map((account) =>
        getTokensBySearchValue(account).length > 0 ? (
          <Column
            fullWidth={true}
            key={account.name}
          >
            <AccountSection
              width='100%'
              padding='0px 8px 12px 8px'
              marginBottom={11}
              justifyContent='space-between'
            >
              <Row width='unset'>
                <AccountNameText textSize='14px'>
                  {account.name}
                </AccountNameText>
                <AccountAddressText textSize='14px'>
                  {reduceAddress(account.address)}
                </AccountAddressText>
              </Row>
              {selectedSendOption === SendPageTabHashes.token && (
                <Row width='unset'>
                  {isLoadingSpotPrices ? (
                    <LoadingSkeleton
                      width={80}
                      height={14}
                    />
                  ) : (
                    <BalanceText textSize='14px'>
                      {getAccountFiatValue(account)}
                    </BalanceText>
                  )}
                </Row>
              )}
            </AccountSection>
            <Column
              fullWidth={true}
              padding='0px 0px 8px 0px'
            >
              {getTokensBySearchValue(account).map((token) => (
                <TokenListItem
                  token={token}
                  onClick={() => handleSelectAsset(token, account)}
                  key={getAssetIdKey(token)}
                  balance={getBalance(
                    account.accountId,
                    token,
                    tokenBalancesRegistry
                  )}
                  isLoadingSpotPrice={isLoadingSpotPrices}
                  disabledText={
                    getIsTokenSelected(token, selectedFromToken)
                      ? 'braveWalletFromToken'
                      : getIsTokenSelected(token, selectedToToken)
                      ? 'braveWalletToToken'
                      : undefined
                  }
                  spotPrice={
                    spotPriceRegistry
                      ? getTokenPriceAmountFromRegistry(
                          spotPriceRegistry,
                          token
                        ).format()
                      : ''
                  }
                />
              ))}
            </Column>
          </Column>
        ) : null
      )
    }, [
      isLoadingBalances,
      showFullFlatTokenList,
      emptyTokensList,
      accounts,
      selectedSendOption,
      handleSelectAsset,
      getTokensBySearchValue,
      isLoadingSpotPrices,
      getAccountFiatValue,
      tokenBalancesRegistry,
      spotPriceRegistry,
      selectedFromToken,
      selectedToToken
    ])

    // computed
    const modalTitle = getLocale(
      modalType === 'swap'
        ? 'braveWalletChooseAssetToSwap'
        : 'braveWalletChooseAssetToSend'
    )

    // render
    return (
      <PopupModal
        onClose={onClose}
        title={modalTitle}
        width='560px'
        borderRadius={16}
        height='90vh'
        ref={forwardedRef}
      >
        {onSelectSendOption && (
          <SendOptionsRow justifyContent='flex-start'>
            <SelectSendOptionButtons
              onClick={onSelectSendOption}
              selectedSendOption={selectedSendOption}
            />
          </SendOptionsRow>
        )}
        {selectedNetworkFilter && (
          <SearchBarRow>
            <NetworkFilterWithSearch
              searchValue={searchValue}
              searchPlaceholder={
                selectedSendOption === SendPageTabHashes.token
                  ? getLocale('braveWalletSearchTokens')
                  : getLocale('braveWalletSearchNFTs')
              }
              searchAction={onSearch}
              searchAutoFocus={true}
              selectedNetwork={selectedNetworkFilter}
              onClick={onToggleShowNetworkDropdown}
              showNetworkDropDown={showNetworkDropDown}
              onSelectNetwork={onSelectAssetsNetwork}
              networkSelectorDisabled={showFullFlatTokenList}
              networkListSubset={
                modalType === 'swap' ? swapNetworks : undefined
              }
            />
          </SearchBarRow>
        )}
        <ScrollContainer
          fullWidth={true}
          noPadding={!!showFullFlatTokenList}
          justifyContent={emptyTokensList ? 'center' : 'flex-start'}
        >
          {tokensByAccount}
        </ScrollContainer>
      </PopupModal>
    )
  }
)
