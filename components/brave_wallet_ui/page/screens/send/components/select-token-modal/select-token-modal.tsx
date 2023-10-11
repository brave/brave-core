// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Selectors
import {
  selectAllVisibleUserAssetsFromQueryResult //
} from '../../../../../common/slices/entities/blockchain-token.entity'
import {
  selectAllAccountInfosFromQuery //
} from '../../../../../common/slices/entities/account-info.entity'

// Types
import {
  BraveWallet,
  CoinTypesMap,
  SendPageTabHashes
} from '../../../../../constants/types'

// Utils
import { getLocale } from '../../../../../../common/locale'
import { getPriceIdForToken } from '../../../../../utils/api-utils'
import { filterNetworksForAccount } from '../../../../../utils/network-utils'
import {
  computeFiatAmount,
  getTokenPriceAmountFromRegistry
} from '../../../../../utils/pricing-utils'
import Amount from '../../../../../utils/amount'
import {
  getBalance
} from '../../../../../utils/balance-utils'
import { getAssetIdKey } from '../../../../../utils/asset-utils'

// Queries
import {
  useGetDefaultFiatCurrencyQuery,
  useGetVisibleNetworksQuery,
  useSetNetworkMutation,
  useGetTokenSpotPricesQuery,
  useSetSelectedAccountMutation,
  useGetUserTokensRegistryQuery,
  useGetAccountInfosRegistryQuery,
} from '../../../../../common/slices/api.slice'
import {
  querySubscriptionOptions60s
} from '../../../../../common/slices/constants'

// hooks
import {
  useBalancesFetcher
} from '../../../../../common/hooks/use-balances-fetcher'

// Options
import { AllNetworksOption } from '../../../../../options/network-filter-options'

// Assets
import CloseIcon from '../../../../../assets/svg-icons/close.svg'

// Components
import { TokenListItem } from '../token-list-item/token-list-item'
import {
  TokenListItemSkeleton
} from '../token-list-item/token_list_item_skeleton'
import {
  LoadingSkeleton
} from '../../../../../components/shared/loading-skeleton/index'
import { NetworkFilterWithSearch } from '../../../../../components/desktop/network-filter-with-search'

// Styled Components
import { Row, Column, Text, VerticalDivider, IconButton, VerticalSpacer } from '../../shared.styles'
import { Wrapper, Modal, ScrollContainer, AccountSection } from './select-tokenmodal.style'

interface Props {
  onClose: () => void
  selectedSendOption: SendPageTabHashes
  selectSendAsset: (asset: BraveWallet.BlockchainToken | undefined) => void
}

export const SelectTokenModal = React.forwardRef<HTMLDivElement, Props>(
  (props: Props, forwardedRef) => {
    const { onClose, selectedSendOption, selectSendAsset } = props

    // State
    const [searchValue, setSearchValue] = React.useState<string>('')
    const [showNetworkDropDown, setShowNetworkDropDown] = React.useState<boolean>(false)
    const [selectedNetworkFilter, setSelectedNetworkFilter] = React.useState<BraveWallet.NetworkInfo>(AllNetworksOption)

    // Queries & Mutations
    const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()
    const [setNetwork] = useSetNetworkMutation()
    const [setSelectedAccount] = useSetSelectedAccountMutation()
    const { data: networks } = useGetVisibleNetworksQuery()
    const { accounts } = useGetAccountInfosRegistryQuery(undefined, {
      selectFromResult: (res) => ({
        accounts: selectAllAccountInfosFromQuery(res)
      })
    })

    const { userVisibleTokensInfo } = useGetUserTokensRegistryQuery(undefined, {
      selectFromResult: result => ({
        userVisibleTokensInfo: selectAllVisibleUserAssetsFromQueryResult(result)
      })
    })

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
        const localHostCoins = userVisibleTokensInfo.filter((token) =>
          token.chainId === BraveWallet.LOCALHOST_CHAIN_ID)
        const accountsLocalHost = localHostCoins.find((token) =>
          token.symbol.toUpperCase() === coinName)

      const chainList = filterNetworksForAccount(networks, account.accountId).map(
        (network) => network.chainId
      )

        const list = userVisibleTokensInfo.filter(
          (token) =>
            token.chainId !== BraveWallet.LOCALHOST_CHAIN_ID &&
            chainList.includes(token?.chainId ?? '')
        )

        if (
          accountsLocalHost &&
          (account.accountId.keyringId !== BraveWallet.KeyringId.kFilecoin)
        ) {
          list.push(accountsLocalHost)
          return list
        }

        return list.filter((token) => token.visible)
      },
      [userVisibleTokensInfo, networks]
    )

    const {
      data: tokenBalancesRegistry,
      isLoading: isLoadingBalances
    } = useBalancesFetcher({
      accounts,
      networks
    })

    const getTokenListWithBalances = React.useCallback(
      (account: BraveWallet.AccountInfo) => {
        return getTokenListByAccount(account)
          .filter(token => new Amount(
            getBalance(account.accountId, token, tokenBalancesRegistry)).gt(0))
      },
      [getTokenListByAccount, tokenBalancesRegistry]
    )

    const getTokensBySelectedSendOption = React.useCallback(
      (account: BraveWallet.AccountInfo) => {
        if (selectedSendOption === SendPageTabHashes.nft) {
          return getTokenListWithBalances(account).filter(token =>
            token.isErc721 || token.isNft || token.isErc1155)
        }
        return getTokenListWithBalances(account).filter(token =>
          !token.isErc721 && !token.isErc1155 && !token.isNft)
      },
      [getTokenListWithBalances, selectedSendOption]
    )

    const tokenPriceIds = React.useMemo(() =>
      accounts
        .flatMap(getTokensBySelectedSendOption)
        .filter(token => !token.isErc721 && !token.isErc1155 && !token.isNft)
        .map(getPriceIdForToken),
      [accounts, getTokensBySelectedSendOption]
    )
    const {
      data: spotPriceRegistry
    } = useGetTokenSpotPricesQuery(
      !isLoadingBalances && tokenPriceIds.length && defaultFiatCurrency
        ? { ids: tokenPriceIds, toCurrency: defaultFiatCurrency }
        : skipToken,
      querySubscriptionOptions60s
    )

    const getTokensByNetwork = React.useCallback(
      (account: BraveWallet.AccountInfo) => {
        if (selectedNetworkFilter.chainId === AllNetworksOption.chainId) {
          return getTokensBySelectedSendOption(account)
        }
        return getTokensBySelectedSendOption(account).filter((token) =>
          token.chainId === selectedNetworkFilter.chainId &&
          token.coin === selectedNetworkFilter.coin
        )
      },
      [
        getTokensBySelectedSendOption,
        selectedNetworkFilter.chainId,
        selectedNetworkFilter.coin
      ]
    )

    const getTokensBySearchValue = React.useCallback(
      (account: BraveWallet.AccountInfo) => {
        if (searchValue === '') {
          return getTokensByNetwork(account)
        }
        return getTokensByNetwork(account).filter((token) =>
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
          return a !== '' && b !== ''
            ? new Amount(a).plus(b).format()
            : ''
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

    const onSelectSendAsset = React.useCallback(
      async (
        token: BraveWallet.BlockchainToken,
        account: BraveWallet.AccountInfo
      ) => {
        selectSendAsset(token)
        await setSelectedAccount(account.accountId)
        await setNetwork({
          chainId: token.chainId,
          coin: token.coin
        })
        onClose()
      },
      [selectSendAsset, onClose, setNetwork]
    )

    const onSearch = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
      setSearchValue(event.target.value)
    }, [])

    const onToggleShowNetworkDropdown = React.useCallback(() => {
      setShowNetworkDropDown((prev) => !prev)
    }, [])

    const onSelectAssetsNetwork = React.useCallback((network: BraveWallet.NetworkInfo) => {
      setSelectedNetworkFilter(network)
      setShowNetworkDropDown(false)
    }, [])

    // Memos
    const emptyTokensList = React.useMemo(() => {
      return !isLoadingBalances &&
        accounts.map(
          (account) =>
            getTokensBySearchValue(account)
        ).flat(1).length === 0
    }, [accounts, getTokensBySearchValue, isLoadingBalances])

    const tokensByAccount = React.useMemo(() => {
      if (isLoadingBalances) {
        return (
          <Column columnWidth='full'>
            <AccountSection
              rowWidth='full'
              verticalPadding={9}
              horizontalPadding={16}
            >
              <LoadingSkeleton width={80} height={14} />
            </AccountSection>
            <Column
              columnWidth='full'
              horizontalPadding={8}>
              <TokenListItemSkeleton
                isNFT={selectedSendOption === SendPageTabHashes.nft}
              />
            </Column>
          </Column>
        )
      }
      if (emptyTokensList) {
        return <Text textSize='14px' isBold={false} textColor='text03'>
          {getLocale('braveWalletNoAvailableTokens')}
        </Text>
      }

      return accounts.map((account) =>
        getTokensBySearchValue(account).length > 0 ? (
          <Column columnWidth='full' key={account.name}>
            <AccountSection
              rowWidth='full'
              verticalPadding={6}
              horizontalPadding={16}
            >
              <Text textColor='text02' textSize='14px' isBold={true}>
                {account.name}
              </Text>
              {selectedSendOption === SendPageTabHashes.token && (
                <Text textColor='text03' textSize='14px' isBold={false}>
                  {getAccountFiatValue(account)}
                </Text>
              )}
            </AccountSection>
            <Column columnWidth='full' horizontalPadding={8}>
              <VerticalSpacer size={8} />
              {getTokensBySearchValue(account).map((token) => (
                <TokenListItem
                  token={token}
                  onClick={() => onSelectSendAsset(token, account)}
                  key={getAssetIdKey(token)}
                  balance={getBalance(
                    account.accountId,
                    token,
                    tokenBalancesRegistry
                  )}
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
      accounts,
      onSelectSendAsset,
      getTokensBySearchValue,
      getAccountFiatValue,
      emptyTokensList,
      selectedSendOption,
      tokenBalancesRegistry,
      isLoadingBalances
    ])

    // computed
    const modalTitle = getLocale(
      selectedSendOption === SendPageTabHashes.nft
        ? 'braveWalletSendTabSelectNFTTitle'
        : 'braveWalletSendTabSelectTokenTitle'
    )

    // render
    return (
      <Wrapper>
        <Modal ref={forwardedRef}>
          <Row rowWidth='full' horizontalPadding={24} verticalPadding={20}>
            <Text textSize='18px' isBold={true}>
              {modalTitle}
            </Text>
            <IconButton icon={CloseIcon} onClick={onClose} size={20} />
          </Row>
          <Row rowWidth='full' horizontalPadding={16} marginBottom={16}>
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
            />
          </Row>
          <VerticalDivider />
          <ScrollContainer
            columnWidth='full'
            verticalAlign={emptyTokensList ? 'center' : 'flex-start'}
          >
            {tokensByAccount}
          </ScrollContainer>
        </Modal>
      </Wrapper>
    )
  }
)
