// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'
import { Redirect, Route, Switch, useHistory, useParams } from 'react-router'
import type { VariableSizeList as List } from 'react-window'

// Selectors
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../common/selectors'

// utils
import { getLocale } from '../../../../common/locale'
import {
  getAssetIdKey,
  getRampAssetSymbol,
  isSelectedAssetInAssetOptions
} from '../../../utils/asset-utils'

// types
import {
  BraveWallet,
  BuyOption,
  NetworkFilterType,
  WalletRoutes
} from '../../../constants/types'

// options
import { AllNetworksOption } from '../../../options/network-filter-options'
import { SelectBuyOption } from '../../../components/buy-send-swap/select-buy-option/select-buy-option'

// hooks
import {
  useGetBuyUrlQuery,
  useGetDefaultFiatCurrencyQuery,
  useGetNetworkQuery,
  useGetOnRampAssetsQuery,
  useGetOnRampFiatCurrenciesQuery,
  useGetOnRampNetworksQuery,
  useLazyGetBuyUrlQuery
} from '../../../common/slices/api.slice'
import {
  useAccountsQuery,
  useReceiveAddressQuery
} from '../../../common/slices/api.slice.extra'

// style
import {
  Column,
  Flex,
  LoadingIcon,
  Row,
  VerticalSpace
} from '../../../components/shared/style'
import { NextButtonRow } from '../onboarding/onboarding.style'
import {
  ScrollContainer,
  SearchWrapper,
  SelectAssetWrapper,
  Title,
  Description,
  Divider,
  Alert,
  AlertText,
  InfoIcon
} from './fund-wallet.style'

// components
import {
  RenderTokenFunc,
  VirtualizedTokensList
} from '../../../components/desktop/views/portfolio/components/token-lists/virtualized-tokens-list'
import SearchBar from '../../../components/shared/search-bar'
import SelectAccountItem from '../../../components/shared/select-account-item'
import SelectAccount from '../../../components/shared/select-account'
import { BuyAssetOptionItem } from '../../../components/shared/buy-option/buy-asset-option'
import { NavButton } from '../../../components/extension/buttons/nav-button/index'
import CreateAccountTab from '../../../components/buy-send-swap/create-account'
import {
  BuyAmountInput //
} from '../../../components/buy-send-swap/buy_amount_input/buy_amount_input'
import SelectHeader from '../../../components/buy-send-swap/select-header'
import {
  SelectOnRampFiatCurrency //
} from '../../../components/buy-send-swap/select-currency/select-currency'
import WalletPageWrapper from '../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper'
import { PageTitleHeader } from '../../../components/desktop/card-headers/page-title-header'
import {
  FilterTokenRow //
} from '../../../components/desktop/views/portfolio/style'
import {
  NetworkFilterSelector //
} from '../../../components/desktop/network-filter-selector'
import { BuyOptions } from '../../../options/buy-with-options'
import {
  makeFundWalletPurchaseOptionsRoute,
  makeFundWalletRoute
} from '../../../utils/routes-utils'

const itemSize = 82

function getItemSize(index: number): number {
  return itemSize
}

const getItemKey = (i: number, data: BraveWallet.BlockchainToken[]) =>
  getAssetIdKey(data[i])

interface Props {
  isAndroid?: boolean
}
interface Params {
  assetId: string
}

export const FundWalletScreen = ({ isAndroid }: Props) => {
  // render
  return (
    <Switch>
      <Route
        path={WalletRoutes.FundWalletPurchaseOptionsPage}
        exact
      >
        <PurchaseOptionSelection isAndroid={isAndroid} />
      </Route>

      <Route
        path={WalletRoutes.FundWalletPage}
        exact
      >
        <AssetSelection isAndroid={isAndroid} />
      </Route>

      <Redirect to={WalletRoutes.FundWalletPage} />
    </Switch>
  )
}

function AssetSelection({ isAndroid }: Props) {
  // routing
  const history = useHistory()
  const { assetId: selectedOnRampAssetId } = useParams<Params>()
  const params = new URLSearchParams(history.location.search)
  const currencyCode = params.get('currencyCode')
  const buyAmountParam = params.get('buyAmount')
  const searchParam = params.get('search')
  const chainIdParam = params.get('chainId')
  const coinTypeParam = params.get('coinType')

  // redux
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // state
  const [selectedCurrency, setSelectedCurrency] = React.useState<string>(
    currencyCode || 'USD'
  )
  const [searchValue, setSearchValue] = React.useState<string>(
    searchParam ?? ''
  )
  const [showFiatSelection, setShowFiatSelection] =
    React.useState<boolean>(false)
  const [buyAmount, setBuyAmount] = React.useState<string>(buyAmountParam || '')
  const [selectedNetworkFilter, setSelectedNetworkFilter] =
    React.useState<NetworkFilterType>(
      chainIdParam && coinTypeParam !== null
        ? {
            chainId: chainIdParam,
            coin: Number(coinTypeParam)
          }
        : AllNetworksOption
    )

  // queries
  const { data: defaultFiatCurrency = 'USD' } = useGetDefaultFiatCurrencyQuery(
    undefined,
    {
      selectFromResult: (res) => ({
        data: res.data?.toUpperCase()
      })
    }
  )
  const { data: buyAssetNetworks = [] } = useGetOnRampNetworksQuery()
  const { data: selectedNetworkFromFilter = AllNetworksOption } =
    useGetNetworkQuery(
      !selectedNetworkFilter ||
        selectedNetworkFilter.chainId === AllNetworksOption.chainId
        ? skipToken
        : selectedNetworkFilter
    )
  const { data: onRampAssets } = useGetOnRampAssetsQuery()
  const selectedAsset = onRampAssets?.allAssetOptions.find(
    (opt) => getAssetIdKey(opt) === selectedOnRampAssetId
  )

  const { data: selectedNetwork = AllNetworksOption } = useGetNetworkQuery(
    selectedAsset ||
      (selectedNetworkFilter.chainId === AllNetworksOption.chainId
        ? skipToken
        : selectedNetworkFilter)
  )

  const { data: options } = useGetOnRampAssetsQuery()
  const allBuyAssetOptions = options?.allAssetOptions || []

  // refs
  const listRef = React.useRef<List<BraveWallet.BlockchainToken[]>>(null)

  // methods
  // This filters a list of assets when the user types in search bar
  const onSearchValueChange = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      setSearchValue(event.target.value)
    },
    []
  )

  const renderToken: RenderTokenFunc<BraveWallet.BlockchainToken> =
    React.useCallback(
      ({ item: asset }) => {
        const assetId = getAssetIdKey(asset)
        return (
          <BuyAssetOptionItem
            selectedCurrency={selectedCurrency}
            key={assetId}
            token={asset}
            onClick={() => history.push(makeFundWalletRoute(assetId))}
          />
        )
      },
      [selectedCurrency]
    )

  // memos & computed
  const assetsForFilteredNetwork = React.useMemo(() => {
    if (!allBuyAssetOptions) {
      return []
    }

    const assets =
      selectedNetworkFilter.chainId === AllNetworksOption.chainId
        ? allBuyAssetOptions
        : allBuyAssetOptions.filter(
            ({ chainId }) => selectedNetworkFilter.chainId === chainId
          )

    return assets
  }, [selectedNetworkFilter.chainId, allBuyAssetOptions])

  const assetListSearchResults = React.useMemo(() => {
    if (searchValue === '') {
      return assetsForFilteredNetwork
    }
    return assetsForFilteredNetwork.filter((asset) => {
      const searchValueLower = searchValue.toLowerCase()
      return (
        asset.name.toLowerCase().startsWith(searchValueLower) ||
        asset.symbol.toLowerCase().startsWith(searchValueLower)
      )
    })
  }, [searchValue, assetsForFilteredNetwork])

  const assetsUI = React.useMemo(
    () =>
      allBuyAssetOptions?.length ? (
        <VirtualizedTokensList
          listRef={listRef}
          getItemKey={getItemKey}
          getItemSize={getItemSize}
          userAssetList={assetListSearchResults}
          estimatedItemSize={itemSize}
          renderToken={renderToken}
          maximumViewableTokens={isPanel ? 2.5 : 4.5}
        />
      ) : (
        <Column>
          <LoadingIcon
            opacity={1}
            size='100px'
            color='interactive05'
          />
        </Column>
      ),
    [assetListSearchResults, renderToken, isPanel]
  )

  const networksFilterOptions: BraveWallet.NetworkInfo[] = React.useMemo(() => {
    return [AllNetworksOption, ...buyAssetNetworks]
  }, [buyAssetNetworks])

  // computed
  const isNextStepEnabled = !!selectedAsset
  const pageTitle = `${getLocale('braveWalletBuy')}${
    selectedAsset ? ` ${selectedAsset.symbol}` : ''
  }`

  // effects
  React.useEffect(() => {
    // scroll selected item into view
    if (listRef.current && selectedOnRampAssetId) {
      const itemIndex = assetListSearchResults.findIndex(
        (asset) => getAssetIdKey(asset) === selectedOnRampAssetId
      )
      if (itemIndex > -1) {
        listRef.current.scrollToItem(itemIndex)
      }
    }
  }, [selectedOnRampAssetId, assetListSearchResults, listRef])

  React.useEffect(() => {
    // initialize selected currency
    if (defaultFiatCurrency) {
      setSelectedCurrency(defaultFiatCurrency)
    }
  }, [defaultFiatCurrency])

  // render
  if (showFiatSelection) {
    return (
      <WalletPageWrapper
        wrapContentInBox={true}
        hideNav={isAndroid}
        hideHeader={isAndroid}
        cardHeader={
          <PageTitleHeader
            title={pageTitle}
            showBackButton
            onBack={() => setShowFiatSelection(false)}
          />
        }
      >
        <Column
          padding='0 12px'
          fullWidth
        >
          <SelectOnRampFiatCurrency
            onSelectCurrency={(currency) => {
              setSelectedCurrency(currency.currencyCode.toUpperCase())
              setShowFiatSelection(false)
            }}
            onBack={() => setShowFiatSelection(false)}
          />
        </Column>
      </WalletPageWrapper>
    )
  }

  return (
    <WalletPageWrapper
      wrapContentInBox={true}
      hideNav={isAndroid}
      hideHeader={isAndroid}
      cardHeader={<PageTitleHeader title={pageTitle} />}
    >
      <Column
        padding='0 12px'
        fullWidth
      >
        <SelectAssetWrapper>
          <Row marginBottom={8}>
            <BuyAmountInput
              onAmountChange={setBuyAmount}
              buyAmount={buyAmount}
              selectedAsset={selectedAsset}
              selectedNetwork={selectedNetwork}
              autoFocus={true}
              onShowCurrencySelection={() => setShowFiatSelection(true)}
              selectedFiatCurrencyCode={selectedCurrency}
            />
          </Row>

          <FilterTokenRow
            horizontalPadding={0}
            isV2={false}
          >
            <Column
              flex={1}
              style={{ minWidth: '25%' }}
              alignItems='flex-start'
            >
              <SearchBar
                placeholder={getLocale('braveWalletSearchText')}
                action={onSearchValueChange}
                value={searchValue}
                isV2={false}
              />
            </Column>
            <NetworkFilterSelector
              isV2={false}
              networkListSubset={networksFilterOptions}
              selectedNetwork={selectedNetworkFromFilter}
              onSelectNetwork={setSelectedNetworkFilter}
            />
          </FilterTokenRow>

          {assetsUI}

          <VerticalSpace space='24px' />
        </SelectAssetWrapper>

        <NextButtonRow>
          <NavButton
            buttonType='primary'
            text={
              selectedAsset
                ? getLocale('braveWalletBuyContinueButton')
                : getLocale('braveWalletBuySelectAsset')
            }
            onSubmit={() => {
              if (!selectedOnRampAssetId) {
                return
              }

              const searchValueLower = searchValue.toLowerCase()

              // save latest form values in router history
              history.replace(
                makeFundWalletRoute(selectedOnRampAssetId, {
                  currencyCode: selectedCurrency,
                  buyAmount,
                  // save latest search-box value (if it matches selection name
                  // or symbol)
                  searchText:
                    searchValue &&
                    (selectedAsset?.name
                      .toLowerCase()
                      .startsWith(searchValueLower) ||
                      selectedAsset?.symbol
                        .toLowerCase()
                        .startsWith(searchValueLower))
                      ? searchValue
                      : undefined,
                  // saving network filter (if it matches selection)
                  chainId:
                    selectedAsset?.chainId === selectedNetworkFilter.chainId
                      ? selectedNetworkFilter.chainId ||
                        AllNetworksOption.chainId
                      : AllNetworksOption.chainId,
                  coinType:
                    selectedAsset?.coin === selectedNetworkFilter.coin
                      ? selectedNetworkFilter.coin.toString() ||
                        AllNetworksOption.coin.toString()
                      : AllNetworksOption.coin.toString()
                })
              )

              // go to payment option selection
              history.push(
                makeFundWalletPurchaseOptionsRoute(selectedOnRampAssetId, {
                  buyAmount: buyAmount || '0',
                  currencyCode: selectedCurrency
                })
              )
            }}
            disabled={!isNextStepEnabled}
            isV2={true}
            minWidth='360px'
          />
        </NextButtonRow>
      </Column>
    </WalletPageWrapper>
  )
}

function PurchaseOptionSelection({ isAndroid }: Props) {
  // routing
  const history = useHistory()
  const { assetId: selectedOnRampAssetId } = useParams<Params>()
  const params = new URLSearchParams(history.location.search)
  const currencyCodeParam = params.get('currencyCode')
  const buyAmountParam = params.get('buyAmount')

  // queries
  const { accounts } = useAccountsQuery()
  const { data: fiatCurrencies = [] } = useGetOnRampFiatCurrenciesQuery()
  const selectedCurrency = fiatCurrencies.find(
    (c) => c.currencyCode === currencyCodeParam
  )
  const currencyCode = selectedCurrency ? selectedCurrency.currencyCode : 'USD'

  const { data: options } = useGetOnRampAssetsQuery()
  const selectedAsset = options?.allAssetOptions.find(
    (opt) => getAssetIdKey(opt) === selectedOnRampAssetId
  )

  const { data: assetNetwork } = useGetNetworkQuery(selectedAsset || skipToken)

  const [getBuyUrl] = useLazyGetBuyUrlQuery()

  const accountsForSelectedAssetCoinType = React.useMemo(() => {
    return selectedAsset
      ? selectedAsset.coin === BraveWallet.CoinType.FIL
        ? accounts.filter((a) =>
            a.accountId.coin === selectedAsset.coin &&
            selectedAsset.chainId === BraveWallet.FILECOIN_TESTNET
              ? a.accountId.address.startsWith('t')
              : !a.accountId.address.startsWith('t')
          )
        : accounts.filter((a) => a.accountId.coin === selectedAsset.coin)
      : []
  }, [selectedAsset, accounts])

  // state
  const [showAccountSearch, setShowAccountSearch] =
    React.useState<boolean>(false)
  const [accountSearchText, setAccountSearchText] = React.useState<string>('')
  const [selectedAccount, setSelectedAccount] = React.useState<
    BraveWallet.AccountInfo | undefined
  >(accountsForSelectedAssetCoinType[0])

  // state-dependant queries
  const generatedAddress = useReceiveAddressQuery(selectedAccount?.accountId)

  const { data: buyWithStripeUrl } = useGetBuyUrlQuery(
    selectedAsset && assetNetwork && generatedAddress
      ? {
          assetSymbol: selectedAsset.symbol.toLowerCase(),
          onRampProvider: BraveWallet.OnRampProvider.kStripe,
          chainId: assetNetwork.chainId,
          address: generatedAddress,
          amount: buyAmountParam || '0',
          currencyCode: currencyCode.toLowerCase()
        }
      : skipToken
  )

  // memos
  const accountsForSelectedAssetNetwork = React.useMemo(() => {
    return assetNetwork
      ? accounts.filter((a) => a.accountId.coin === assetNetwork.coin)
      : []
  }, [assetNetwork, accounts])

  const accountListSearchResults = React.useMemo(() => {
    if (accountSearchText === '') {
      return accountsForSelectedAssetNetwork
    }

    return accountsForSelectedAssetNetwork.filter((item) => {
      return item.name.toLowerCase().startsWith(accountSearchText.toLowerCase())
    })
  }, [accountSearchText, accountsForSelectedAssetNetwork])

  const onRampAssetMap = React.useMemo(() => {
    if (!options) {
      return undefined
    }

    return {
      [BraveWallet.OnRampProvider.kRamp]: options.rampAssetOptions,
      [BraveWallet.OnRampProvider.kSardine]: options.sardineAssetOptions,
      [BraveWallet.OnRampProvider.kTransak]: options.transakAssetOptions,
      [BraveWallet.OnRampProvider.kStripe]: options.stripeAssetOptions,
      [BraveWallet.OnRampProvider.kCoinbase]: options.coinbaseAssetOptions
    }
  }, [options])

  const selectedAssetBuyOptions: BuyOption[] = React.useMemo(() => {
    if (!onRampAssetMap) {
      return []
    }

    return selectedAsset
      ? BuyOptions.filter((buyOption) =>
          isSelectedAssetInAssetOptions(
            selectedAsset,
            onRampAssetMap[buyOption.id]
          )
        )
          .filter((buyOption) => {
            if (buyOption.id === BraveWallet.OnRampProvider.kStripe) {
              // hide the option if Stripe URL could not be created
              return buyWithStripeUrl
            }
            return true
          })
          .sort((optionA, optionB) => optionA.name.localeCompare(optionB.name))
      : []
  }, [selectedAsset, onRampAssetMap, buyWithStripeUrl])

  // computed
  const needsAccount: boolean =
    !!selectedAsset && accountsForSelectedAssetNetwork.length < 1

  const pageTitle = selectedAsset
    ? `${getLocale('braveWalletBuy')} ${selectedAsset.symbol}`
    : getLocale('braveWalletBuy')

  // methods
  const openAccountSearch = React.useCallback(
    () => setShowAccountSearch(true),
    []
  )
  const closeAccountSearch = React.useCallback(
    () => setShowAccountSearch(false),
    []
  )
  const onSearchTextChanged = React.useCallback(
    (e: React.ChangeEvent<HTMLInputElement>) =>
      setAccountSearchText(e.target.value),
    []
  )

  const onSelectAccountFromSearch = React.useCallback(
    (account: BraveWallet.AccountInfo) => {
      closeAccountSearch()
      setSelectedAccount(account)
    },
    [closeAccountSearch]
  )

  const openBuyAssetLink = React.useCallback(
    async (buyOption: BraveWallet.OnRampProvider) => {
      if (!selectedAsset || !assetNetwork || !generatedAddress) {
        return
      }

      try {
        const url = await getBuyUrl({
          assetSymbol:
            buyOption === BraveWallet.OnRampProvider.kRamp
              ? getRampAssetSymbol(selectedAsset)
              : buyOption === BraveWallet.OnRampProvider.kStripe
              ? selectedAsset.symbol.toLowerCase()
              : selectedAsset.symbol,
          onRampProvider: buyOption,
          chainId: assetNetwork.chainId,
          address: generatedAddress,
          amount: buyAmountParam || '0',
          currencyCode:
            buyOption === BraveWallet.OnRampProvider.kStripe
              ? currencyCode.toLowerCase()
              : currencyCode
        }).unwrap()

        if (url && chrome.tabs !== undefined) {
          chrome.tabs.create({ url }, () => {
            if (chrome.runtime.lastError) {
              console.error(
                'tabs.create failed: ' + chrome.runtime.lastError.message
              )
            }
          })
        } else {
          // Tabs.create is desktop specific. Using window.open for android.
          window.open(url, '_blank', 'noopener')
        }
      } catch (error) {
        console.error(error)
      }
    },
    [
      buyAmountParam,
      selectedAsset,
      assetNetwork,
      getBuyUrl,
      params,
      currencyCode,
      generatedAddress
    ]
  )

  // effects
  React.useEffect(() => {
    // force selected account option state
    setSelectedAccount(accountsForSelectedAssetCoinType[0])
  }, [accountsForSelectedAssetCoinType[0]])

  // render
  if (!selectedOnRampAssetId) {
    return <Redirect to={WalletRoutes.FundWalletPageStart} />
  }

  return (
    <WalletPageWrapper
      wrapContentInBox={true}
      hideNav={isAndroid}
      hideHeader={isAndroid}
      cardHeader={
        <PageTitleHeader
          title={pageTitle}
          showBackButton
          onBack={history.goBack}
        />
      }
    >
      <Column
        padding='0 12px'
        fullWidth
      >
        {needsAccount && assetNetwork ? (
          // Creates wallet Account if needed
          <CreateAccountTab
            network={assetNetwork}
            onCancel={history.goBack}
            onCreated={setSelectedAccount}
          />
        ) : showAccountSearch || !selectedAccount ? (
          <SearchWrapper>
            <SelectHeader
              title={getLocale('braveWalletSelectAccount')}
              onBack={closeAccountSearch}
              hasAddButton={false}
            />
            <SearchBar
              placeholder={getLocale('braveWalletSearchAccount')}
              action={onSearchTextChanged}
              isV2={true}
            />
            <VerticalSpace space='16px' />

            <ScrollContainer>
              <SelectAccount
                accounts={accountListSearchResults}
                selectedAccount={selectedAccount}
                onSelectAccount={onSelectAccountFromSearch}
              />
            </ScrollContainer>
          </SearchWrapper>
        ) : (
          // Buy Option selection & Account selection/search
          <>
            <Title>
              {getLocale('braveWalletFundWalletTitle').replace(
                '$1',
                selectedAsset?.symbol ?? ''
              )}
            </Title>

            <Description>
              {getLocale('braveWalletFundWalletDescription')}
            </Description>

            <VerticalSpace space='16px' />

            <Row
              justifyContent='space-around'
              alignItems='center'
            >
              <Flex>
                <SelectAccountItem
                  selectedNetwork={assetNetwork}
                  account={selectedAccount}
                  onSelectAccount={openAccountSearch}
                  showTooltips
                  showSwitchAccountsIcon
                  isV2={true}
                />
              </Flex>
            </Row>

            <VerticalSpace space='6px' />
            <Divider />

            <SelectBuyOption
              layoutType='loose'
              buyOptions={selectedAssetBuyOptions}
              onSelect={openBuyAssetLink}
              selectedOption={undefined}
            />

            <Alert>
              <InfoIcon name='info-filled' />
              <AlertText>{getLocale('braveWalletBuyDisclaimer')}</AlertText>
            </Alert>
          </>
        )}
      </Column>
    </WalletPageWrapper>
  )
}

export default FundWalletScreen
