// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import { skipToken } from '@reduxjs/toolkit/query/react'
import { Redirect, Route, Switch, useHistory, useParams } from 'react-router'

// Selectors
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'
import { UISelectors, WalletSelectors } from '../../../common/selectors'

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
import { useScrollIntoView } from '../../../common/hooks/use-scroll-into-view'

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
import { WalletActions } from '../../../common/actions'
import {
  makeFundWalletPurchaseOptionsRoute,
  makeFundWalletRoute
} from '../../../utils/routes-utils'
import { networkSupportsAccount } from '../../../utils/network-utils'

const itemSize = 82

function getItemSize(index: number): number {
  return itemSize
}

const getItemKey = (i: number, data: BraveWallet.BlockchainToken[]) =>
  getAssetIdKey(data[i])

interface Props {
  isAndroid?: boolean
}

export const FundWalletScreen = ({ isAndroid }: Props) => {
  // redux
  const dispatch = useDispatch()

  // clear selected asset on page mount
  React.useEffect(() => {
    dispatch(WalletActions.selectOnRampAssetId(undefined))
  }, [])

  // render
  return (
    <Switch>
      <Route
        path={WalletRoutes.FundWalletPurchaseOptionsPage}
        exact
      >
        <PurchaseOptionSelection isAndroid={isAndroid} />
      </Route>

      <Route>
        <AssetSelection isAndroid={isAndroid} />
      </Route>
    </Switch>
  )
}

function AssetSelection({ isAndroid }: Props) {
  // router
  const { currencyCode, buyAmount: buyAmountParam } = useParams<{
    currencyCode?: string
    buyAmount?: string
  }>()
  const history = useHistory()
  const params = new URLSearchParams(history.location.search)
  const searchParam = params.get('search')
  const chainIdParam = params.get('chainId')
  const coinTypeParam = params.get('coinType')

  // redux
  const dispatch = useDispatch()
  const isPanel = useSafeUISelector(UISelectors.isPanel)
  const selectedOnRampAssetId = useSelector(
    WalletSelectors.selectedOnRampAssetId
  )

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
  const listItemRefs = React.useRef<Map<string, HTMLButtonElement> | null>(null)

  const getRefsMap = React.useCallback(
    function () {
      if (!listItemRefs.current) {
        // Initialize the Map on first usage.
        listItemRefs.current = new Map()
      }
      return listItemRefs.current
    },
    [listItemRefs]
  )

  // custom hooks
  const scrollIntoView = useScrollIntoView()

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
            ref={(node) => {
              const refs = getRefsMap()
              if (node) {
                refs.set(assetId, node)
              } else {
                refs.delete(assetId)
              }
            }}
            selectedCurrency={selectedCurrency}
            key={assetId}
            token={asset}
            onClick={() => dispatch(WalletActions.selectOnRampAssetId(assetId))}
          />
        )
      },
      [selectedCurrency, getRefsMap, dispatch]
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
    if (selectedOnRampAssetId) {
      const ref = getRefsMap().get(selectedOnRampAssetId)
      if (ref) {
        scrollIntoView(ref, true)
      }
    }
  }, [selectedOnRampAssetId, getRefsMap, scrollIntoView])

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
              const searchValueLower = searchValue.toLowerCase()

              // save latest form values in router history
              history.replace(
                makeFundWalletRoute(
                  selectedCurrency,
                  buyAmount,
                  // save latest search-box value (if it matches selection name
                  // or symbol)
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
                  selectedAsset?.chainId === selectedNetworkFilter.chainId
                    ? selectedNetworkFilter.chainId || AllNetworksOption.chainId
                    : AllNetworksOption.chainId,
                  selectedAsset?.coin === selectedNetworkFilter.coin
                    ? selectedNetworkFilter.coin.toString() ||
                        AllNetworksOption.coin.toString()
                    : AllNetworksOption.coin.toString()
                )
              )

              // go to payment option selection
              history.push(
                makeFundWalletPurchaseOptionsRoute(
                  selectedCurrency,
                  buyAmount || '0'
                )
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
  const params = useParams<{
    currencyCode: string
    buyAmount: string
  }>()
  // redux
  const selectedOnRampAssetId = useSelector(
    WalletSelectors.selectedOnRampAssetId
  )

  // queries
  const { accounts } = useAccountsQuery()
  const { data: fiatCurrencies = [] } = useGetOnRampFiatCurrenciesQuery()
  const selectedCurrency = fiatCurrencies.find(
    (c) => c.currencyCode === params.currencyCode
  )
  const currencyCode = selectedCurrency ? selectedCurrency.currencyCode : 'USD'

  const { data: options } = useGetOnRampAssetsQuery()
  const selectedAsset = options?.allAssetOptions.find(
    (opt) => getAssetIdKey(opt) === selectedOnRampAssetId
  )

  const { data: assetNetwork } = useGetNetworkQuery(selectedAsset || skipToken)

  const [getBuyUrl] = useLazyGetBuyUrlQuery()

  const accountsForSelectedAssetNetwork = React.useMemo(() => {
    if (!assetNetwork) {
      return []
    }
    return accounts.filter((a) =>
      networkSupportsAccount(assetNetwork, a.accountId)
    )
  }, [assetNetwork, accounts])

  // state
  const [showAccountSearch, setShowAccountSearch] =
    React.useState<boolean>(false)
  const [accountSearchText, setAccountSearchText] = React.useState<string>('')
  const [selectedAccount, setSelectedAccount] = React.useState<
    BraveWallet.AccountInfo | undefined
  >(accountsForSelectedAssetNetwork[0])

  // state-dependant queries
  const generatedAddress = useReceiveAddressQuery(selectedAccount?.accountId)

  const { data: buyWithStripeUrl } = useGetBuyUrlQuery(
    selectedAsset && assetNetwork && generatedAddress
      ? {
          assetSymbol: selectedAsset.symbol.toLowerCase(),
          onRampProvider: BraveWallet.OnRampProvider.kStripe,
          chainId: assetNetwork.chainId,
          address: generatedAddress,
          amount: params.buyAmount,
          currencyCode: currencyCode.toLowerCase()
        }
      : skipToken
  )

  // memos
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
          amount: params.buyAmount,
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
    setSelectedAccount(accountsForSelectedAssetNetwork[0])
  }, [accountsForSelectedAssetNetwork])

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
          onBack={() => history.back()}
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
            onCancel={() => history.back()}
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
