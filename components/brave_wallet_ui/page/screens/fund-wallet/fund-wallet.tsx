// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'
import { skipToken } from '@reduxjs/toolkit/query/react'
import { useParams } from 'react-router'

// Selectors
import {
  useSafeUISelector
} from '../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../common/selectors'

// utils
import { getLocale } from '../../../../common/locale'
import {
  getAssetIdKey
} from '../../../utils/asset-utils'

// types
import {
  BraveWallet,
  UserAssetInfoType,
  WalletState
} from '../../../constants/types'

// options
import { AllNetworksOption } from '../../../options/network-filter-options'
import { SelectBuyOption } from '../../../components/buy-send-swap/select-buy-option/select-buy-option'

// hooks
import { useMultiChainBuyAssets } from '../../../common/hooks/use-multi-chain-buy-assets'
import { useScrollIntoView } from '../../../common/hooks/use-scroll-into-view'
import { useGetNetworkQuery } from '../../../common/slices/api.slice'

// style
import { Column, Flex, LoadingIcon, Row, VerticalSpace } from '../../../components/shared/style'
import {
  NextButtonRow
} from '../onboarding/onboarding.style'
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
import SwapInputComponent from '../../../components/buy-send-swap/swap-input-component'
import SelectHeader from '../../../components/buy-send-swap/select-header'
import { SelectCurrency } from '../../../components/buy-send-swap/select-currency/select-currency'
import WalletPageWrapper from '../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper'
import { PageTitleHeader } from '../../../components/desktop/card-headers/page-title-header'
import {
  FilterTokenRow //
} from '../../../components/desktop/views/portfolio/style'
import {
  NetworkFilterSelector //
} from '../../../components/desktop/network-filter-selector'

const itemSize = 82

function getItemSize (index: number): number {
  return itemSize
}

interface Props {
  isAndroid?: boolean
}

export const FundWalletScreen = ({ isAndroid }: Props) => {
  // routing
  const { tokenId } = useParams<{ tokenId?: string }>()

  // redux
  const accounts = useSelector(({ wallet }: { wallet: WalletState }) => wallet.accounts)
  const defaultCurrencies = useSelector(({ wallet }: { wallet: WalletState }) => wallet.defaultCurrencies)
  const selectedNetworkFilter = useSelector(({ wallet }: { wallet: WalletState }) => wallet.selectedNetworkFilter)
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // queries
  const { data: selectedNetworkFromFilter = AllNetworksOption } =
    useGetNetworkQuery(
      !selectedNetworkFilter ||
        selectedNetworkFilter.chainId === AllNetworksOption.chainId
        ? skipToken
        : selectedNetworkFilter
    )

  // custom hooks
  const {
    allAssetOptions: allBuyAssetOptions,
    buyAmount,
    buyAssetNetworks,
    openBuyAssetLink,
    selectedAsset,
    selectedAssetBuyOptions,
    selectedAssetNetwork,
    setBuyAmount,
    setSelectedAsset
  } = useMultiChainBuyAssets()
  const scrollIntoView = useScrollIntoView()

  // state
  const [showFiatSelection, setShowFiatSelection] = React.useState<boolean>(false)
  const [showAccountSearch, setShowAccountSearch] = React.useState<boolean>(false)
  const [accountSearchText, setAccountSearchText] = React.useState<string>('')
  const [selectedCurrency, setSelectedCurrency] = React.useState<string>(defaultCurrencies.fiat || 'usd')
  const [selectedAccount, setSelectedAccount] =
    React.useState<BraveWallet.AccountInfo | undefined>()
  const [showBuyOptions, setShowBuyOptions] = React.useState<boolean>(false)
  const [searchValue, setSearchValue] = React.useState<string>(tokenId ?? '')

  // memos & computed
  const isNextStepEnabled = !!selectedAsset

  const selectedNetwork = selectedAssetNetwork || selectedNetworkFromFilter

  const assetsForFilteredNetwork: UserAssetInfoType[] = React.useMemo(() => {
    if (!allBuyAssetOptions) {
      return []
    }

    const assets = selectedNetworkFilter.chainId === AllNetworksOption.chainId
      ? allBuyAssetOptions
      : allBuyAssetOptions.filter(({ chainId }) =>
        selectedNetworkFilter.chainId === chainId
      )

    return assets.map(asset => ({ asset, assetBalance: '1' }))
  }, [selectedNetworkFilter.chainId, allBuyAssetOptions])

  const assetListSearchResults = React.useMemo(() => {
    if (searchValue === '') {
      return assetsForFilteredNetwork
    }
    return assetsForFilteredNetwork.filter((item) => {
      const searchValueLower = searchValue.toLowerCase()
      return (
        item.asset.name.toLowerCase().startsWith(searchValueLower) ||
        item.asset.symbol.toLowerCase().startsWith(searchValueLower)
      )
    })
  }, [
    searchValue,
    assetsForFilteredNetwork
  ])

  const accountsForSelectedAssetNetwork = React.useMemo(() => {
    return selectedAssetNetwork
      ? accounts.filter(a => a.accountId.coin === selectedAssetNetwork.coin)
      : []
  }, [selectedAssetNetwork, accounts])

  const needsAccount: boolean =
    !!selectedAsset && accountsForSelectedAssetNetwork.length < 1

  const accountListSearchResults = React.useMemo(() => {
    if (accountSearchText === '') {
      return accountsForSelectedAssetNetwork
    }

    return accountsForSelectedAssetNetwork.filter((item) => {
      return item.name.toLowerCase().startsWith(accountSearchText.toLowerCase())
    })
  }, [accountSearchText, accountsForSelectedAssetNetwork])

  const networksFilterOptions: BraveWallet.NetworkInfo[] = React.useMemo(() => {
    return [AllNetworksOption, ...buyAssetNetworks]
  }, [buyAssetNetworks])

  // methods
  const openAccountSearch = React.useCallback(() => setShowAccountSearch(true), [])
  const closeAccountSearch = React.useCallback(() => setShowAccountSearch(false), [])
  const onSearchTextChanged = React.useCallback((e: React.ChangeEvent<HTMLInputElement>) => setAccountSearchText(e.target.value), [])

  // This filters a list of assets when the user types in search bar
  const onSearchValueChange = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      setSearchValue(event.target.value)
    },
    []
  )

  const onSelectAccountFromSearch = React.useCallback(
    (account: BraveWallet.AccountInfo) => {
      closeAccountSearch()
      setSelectedAccount(account)
    },
    [closeAccountSearch]
  )

  const nextStep = React.useCallback(() => {
    if (!isNextStepEnabled) {
      return
    }
    setShowBuyOptions(true)
  }, [isNextStepEnabled])

  const onSubmitBuy = React.useCallback((buyOption: BraveWallet.OnRampProvider) => {
    if (!selectedAsset || !selectedAssetNetwork || !selectedAccount) {
      return
    }
    openBuyAssetLink({
      buyOption,
      depositAddress: selectedAccount.address
    })
  }, [selectedAsset, selectedAssetNetwork, selectedAccount, selectedCurrency])

  const goBackToSelectAssets = React.useCallback(() => {
    setShowBuyOptions(false)
    setSelectedAsset(undefined)
  }, [])

  const onBack = React.useCallback(() => {
    if (showBuyOptions) {
      // go back to asset selection
      goBackToSelectAssets()
    }
  }, [showBuyOptions, goBackToSelectAssets])

  const checkIsBuyAssetSelected = React.useCallback((asset: BraveWallet.BlockchainToken) => {
    if (selectedAsset) {
      return (
        selectedAsset.contractAddress.toLowerCase() === asset.contractAddress.toLowerCase() &&
        selectedAsset.symbol.toLowerCase() === asset.symbol.toLowerCase() &&
        selectedAsset.chainId === asset.chainId
      )
    }
    return false
  }, [selectedAsset])

  const handleScrollIntoView = React.useCallback((asset: BraveWallet.BlockchainToken, ref: HTMLButtonElement | null) => {
    if (checkIsBuyAssetSelected(asset)) {
      scrollIntoView(ref)
    }
  }, [checkIsBuyAssetSelected, scrollIntoView])

  const renderToken: RenderTokenFunc = React.useCallback(({ item: { asset } }) =>
    <BuyAssetOptionItem
      selectedCurrency={selectedCurrency}
      isSelected={checkIsBuyAssetSelected(asset)}
      key={getAssetIdKey(asset)}
      token={asset}
      onClick={setSelectedAsset}
      ref={(ref) => handleScrollIntoView(asset, ref)}
    />,
    [
      selectedCurrency,
      checkIsBuyAssetSelected,
      buyAssetNetworks,
      handleScrollIntoView
    ]
  )

  // effects
  React.useEffect(() => {
    // unselect asset if  AllNetworksOption is not selected
    if (selectedNetworkFilter.chainId !== AllNetworksOption.chainId) {
      setSelectedAsset(undefined)
    }
  }, [selectedNetworkFilter.chainId])

  // sync selected currency with default
  React.useEffect(() => {
    if (defaultCurrencies.fiat) {
      setSelectedCurrency(defaultCurrencies.fiat)
    }
  }, [defaultCurrencies.fiat])

  // sync default selected account with selected asset
  React.useEffect(() => {
    if (
      selectedAsset &&
      selectedAssetNetwork &&
      // asset is selected & account is available
      accountsForSelectedAssetNetwork.length &&
      // needs to change accounts to one with correct network
      selectedAccount?.accountId?.coin !== selectedAsset.coin
    ) {
      setSelectedAccount(accountsForSelectedAssetNetwork[0])
    }
  }, [
    selectedAsset,
    selectedAssetNetwork,
    accountsForSelectedAssetNetwork,
    selectedAccount?.accountId?.coin
  ])

  React.useEffect(() => {
    if(!showBuyOptions && showAccountSearch) {
      closeAccountSearch()
    }
  }, [showBuyOptions, showAccountSearch, closeAccountSearch])

  React.useEffect(() => {
    // reset search field on list update
    if (allBuyAssetOptions && !tokenId) {
      setSearchValue('')
    }
  }, [allBuyAssetOptions, tokenId])

  // render
  return (
    <WalletPageWrapper
      wrapContentInBox={true}
      hideNav={isAndroid}
      hideHeader={isAndroid}
      cardHeader={
        <PageTitleHeader
          title={
            selectedAsset
              ? `${getLocale('braveWalletBuy')} ${selectedAsset.symbol}`
              : getLocale('braveWalletBuy')
          }
          showBackButton={showBuyOptions}
          onBack={onBack}
        />
      }
    >
      <Column padding='0 12px' fullWidth>
        {/* Fiat Selection */}
        {showFiatSelection && (
          <SelectCurrency
            onSelectCurrency={
              // this internally sets the currency via redux,
              // so just hide the UI when selected
              (currency) => {
                setSelectedCurrency(currency.currencyCode)
                setShowFiatSelection(false)
              }
            }
            onBack={() => setShowFiatSelection(false)}
          />
        )}

        {/* Asset Selection */}
        {!showBuyOptions && !showFiatSelection && (
          <>
            <SelectAssetWrapper>
              <Row marginBottom={8}>
                <SwapInputComponent
                  defaultCurrencies={defaultCurrencies}
                  componentType='buyAmount'
                  onInputChange={setBuyAmount}
                  selectedAssetInputAmount={buyAmount}
                  inputName='buy'
                  selectedAsset={selectedAsset}
                  selectedNetwork={selectedNetwork}
                  autoFocus={true}
                  onShowCurrencySelection={() => setShowFiatSelection(true)}
                  isV2={true}
                />
              </Row>

              <FilterTokenRow horizontalPadding={0} isV2={false}>
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
                />
              </FilterTokenRow>

              {allBuyAssetOptions?.length ? (
                <VirtualizedTokensList
                  getItemSize={getItemSize}
                  userAssetList={assetListSearchResults}
                  estimatedItemSize={itemSize}
                  renderToken={renderToken}
                  maximumViewableTokens={isPanel ? 2.5 : 4.5}
                />
              ) : (
                <Column>
                  <LoadingIcon opacity={1} size='100px' color='interactive05' />
                </Column>
              )}

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
                onSubmit={nextStep}
                disabled={!isNextStepEnabled}
                isV2={true}
                minWidth='360px'
              />
            </NextButtonRow>
          </>
        )}

        {/* Creates wallet Account if needed */}
        {needsAccount && showBuyOptions && selectedAssetNetwork && (
          <CreateAccountTab
            network={selectedAssetNetwork}
            onCancel={goBackToSelectAssets}
          />
        )}

        {/* Buy Option selection & Account selection/search */}
        {!needsAccount && showBuyOptions && (
          <>
            {!showAccountSearch && (
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

                <Row justifyContent='space-around' alignItems='center'>
                  <Flex>
                    <SelectAccountItem
                      selectedNetwork={selectedAssetNetwork}
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
                  onSelect={onSubmitBuy}
                  selectedOption={undefined}
                />

                <Alert>
                  <InfoIcon name='info-filled' />
                  <AlertText>{getLocale('braveWalletBuyDisclaimer')}</AlertText>
                </Alert>
              </>
            )}

            {showAccountSearch && (
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
            )}
          </>
        )}
      </Column>
    </WalletPageWrapper>
  )
}

export default FundWalletScreen
