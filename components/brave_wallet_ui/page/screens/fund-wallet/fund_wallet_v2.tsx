// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

// Types
import { WalletRoutes } from '../../../constants/types'

// Hooks
import { useBuy } from './hooks/useBuy'

// Utils
import { getLocale } from '../../../../common/locale'
import { getAssetSymbol } from '../../../utils/meld_utils'
import { isComponentInStorybook } from '../../../utils/string-utils'

// Selectors
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../common/selectors'

// Components
import {
  WalletPageWrapper //
} from '../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper'
import {
  PageTitleHeader //
} from '../../../components/desktop/card-headers/page-title-header'
import {
  PanelActionHeader //
} from '../../../components/desktop/card-headers/panel-action-header'
import {
  SelectAssetButton //
} from './components/select_asset_button/select_asset_button'
import {
  SelectAccountButton //
} from './components/select_account_button/select_account_button'
import { AmountButton } from './components/amount_button/amount_button'
import { SelectCurrency } from './components/select_currency/select_currency'
import { SelectAccount } from './components/select_account/select_account'
import { SelectAsset } from './components/select_asset/select_asset'
import { BuyQuote } from './components/buy_quote/buy_quote'
import { CreateAccount } from './components/create_account/create_account'

// Styled Components
import {
  ContentWrapper,
  ControlPanel,
  Divider,
  Loader,
  LoaderText,
  ServiceProvidersWrapper,
  PaymentMethodIcon,
  SearchAndFilterRow,
  SearchBarWrapper,
  DropdownRow,
  Dropdown,
  InfoIconWrapper,
  InfoIcon
} from './fund_wallet_v2.style'
import { Column, Row, Text } from '../../../components/shared/style'
import { SearchInput } from './components/shared/style'

interface Props {
  isAndroid?: boolean
}

export const FundWalletScreen = ({ isAndroid }: Props) => {
  // State
  const [isCurrencyDialogOpen, setIsCurrencyDialogOpen] = React.useState(false)
  const [isAssetDialogOpen, setIsAssetDialogOpen] = React.useState(false)
  const [isAccountDialogOpen, setIsAccountDialogOpen] = React.useState(false)

  // Hooks
  const {
    selectedAsset,
    selectedCurrency,
    selectedAccount,
    amount,
    isLoadingAssets,
    isLoadingSpotPrices,
    formattedCryptoEstimate,
    spotPriceRegistry,
    fiatCurrencies,
    accounts,
    cryptoCurrencies,
    defaultFiatCurrency,
    isFetchingQuotes,
    quotes,
    filteredQuotes,
    onSelectToken,
    onSelectAccount,
    onSelectCurrency,
    onSetAmount,
    serviceProviders,
    selectedCountryCode,
    isLoadingPaymentMethods,
    isLoadingCountries,
    countries,
    paymentMethods,
    selectedPaymentMethod,
    onSelectPaymentMethod,
    onSelectCountry,
    isCreatingWidgetFor,
    onBuy,
    searchTerm,
    onSearch,
    hasQuoteError,
    showCreateAccount,
    onCloseCreateAccount,
    pendingSelectedToken
  } = useBuy()

  // Redux
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // Computed
  const selectedCountry = countries?.find(
    (country) =>
      country.countryCode.toLowerCase() === selectedCountryCode.toLowerCase()
  )
  const isDarkMode = window.matchMedia('(prefers-color-scheme: dark)').matches
  const isStorybook = isComponentInStorybook()
  const pageTitle = getLocale('braveWalletBuyAsset').replace(
    '$1',
    getAssetSymbol(selectedAsset)
  )
  const isFetchingFirstTimeQuotes = isFetchingQuotes && quotes?.length === 0

  return (
    <>
      <WalletPageWrapper
        wrapContentInBox={true}
        hideNav={isAndroid || isPanel}
        hideHeader={isAndroid}
        cardHeader={
          isPanel ? (
            <PanelActionHeader
              title={pageTitle}
              expandRoute={WalletRoutes.FundWalletPageStart}
            />
          ) : (
            <PageTitleHeader title={pageTitle} />
          )
        }
        useDarkBackground={isPanel}
        noPadding={isPanel}
        noCardPadding={isPanel}
      >
        <ContentWrapper
          width='100%'
          height='100%'
          justifyContent='flex-start'
        >
          <ControlPanel width='100%'>
            <SelectAssetButton
              labelText={getLocale('braveWalletAsset')}
              selectedAsset={selectedAsset}
              onClick={() => setIsAssetDialogOpen(true)}
            />
            <SelectAccountButton
              labelText={getLocale('braveWalletSubviewAccount')}
              selectedAccount={selectedAccount}
              onClick={() => setIsAccountDialogOpen(true)}
            />
            <AmountButton
              labelText={getLocale('braveWalletSwapFrom')}
              currencyCode={
                selectedCurrency?.currencyCode || defaultFiatCurrency
              }
              amount={amount}
              onClick={() => {
                setIsCurrencyDialogOpen(true)
              }}
              onChange={onSetAmount}
              estimatedCryptoAmount={formattedCryptoEstimate}
            />
          </ControlPanel>
          <ServiceProvidersWrapper
            fullWidth={true}
            justifyContent='flex-start'
          >
            <Divider />
            <SearchAndFilterRow
              width='100%'
              justifyContent='space-between'
              alignItems='flex-end'
            >
              <SearchBarWrapper width='100%'>
                <SearchInput
                  placeholder={getLocale('braveWalletSearchText')}
                  value={searchTerm}
                  onInput={(e) => onSearch(e.value)}
                  size='small'
                  disabled={isFetchingFirstTimeQuotes}
                >
                  <Icon
                    name='search'
                    slot='left-icon'
                  />
                </SearchInput>
              </SearchBarWrapper>
              <DropdownRow>
                <Dropdown
                  value={selectedCountryCode}
                  onChange={(detail) => onSelectCountry(detail.value as string)}
                  disabled={isFetchingFirstTimeQuotes || isLoadingCountries}
                >
                  <div slot='value'>{selectedCountry?.name}</div>
                  {countries?.map((country) => {
                    return (
                      <leo-option
                        key={country.countryCode}
                        value={country.countryCode}
                      >
                        {country.name}
                      </leo-option>
                    )
                  })}
                </Dropdown>
                <Dropdown
                  value={selectedPaymentMethod.paymentMethod}
                  onChange={(detail) =>
                    onSelectPaymentMethod(detail.value as string)
                  }
                  disabled={
                    isFetchingFirstTimeQuotes || isLoadingPaymentMethods
                  }
                >
                  <div slot='value'>{selectedPaymentMethod.name}</div>
                  {paymentMethods?.map((paymentMethod) => {
                    const logoUrl = isDarkMode
                      ? paymentMethod.logoImages?.darkUrl
                      : paymentMethod.logoImages?.lightUrl
                    return (
                      <leo-option
                        key={paymentMethod.paymentMethod}
                        value={paymentMethod.paymentMethod}
                      >
                        <Row
                          width='unset'
                          justifyContent='flex-start'
                        >
                          <PaymentMethodIcon
                            src={
                              isStorybook
                                ? logoUrl
                                : `chrome://image?${logoUrl}`
                            }
                          />
                          {paymentMethod.name}
                        </Row>
                      </leo-option>
                    )
                  })}
                </Dropdown>
              </DropdownRow>
            </SearchAndFilterRow>
            {isFetchingFirstTimeQuotes ? (
              <Column
                fullWidth={true}
                height='300px'
              >
                <Loader />
                <LoaderText>
                  {getLocale('braveWalletGettingBestPrices')}
                </LoaderText>
              </Column>
            ) : (
              <>
                {hasQuoteError ? (
                  <Column
                    fullWidth={true}
                    height='300px'
                    gap='8px'
                  >
                    <InfoIconWrapper>
                      <InfoIcon />
                    </InfoIconWrapper>
                    <Text
                      textSize='16px'
                      textColor='primary'
                      isBold={true}
                    >
                      {getLocale('braveWalletNoProviderFound').replace(
                        '$1',
                        getAssetSymbol(selectedAsset)
                      )}
                    </Text>
                    <Text
                      textSize='14px'
                      textColor='secondary'
                      isBold={false}
                    >
                      {getLocale('braveWalletTrySearchingForDifferentAsset')}
                    </Text>
                  </Column>
                ) : (
                  <>
                    {searchTerm !== '' && filteredQuotes.length === 0 ? (
                      <Column
                        fullWidth={true}
                        height='300px'
                        gap='8px'
                      >
                        <InfoIconWrapper>
                          <InfoIcon />
                        </InfoIconWrapper>
                        <Text
                          textSize='16px'
                          textColor='primary'
                          isBold={true}
                        >
                          {getLocale('braveWalletNoResultsFound').replace(
                            '$1',
                            searchTerm
                          )}
                        </Text>
                        <Text
                          textSize='14px'
                          textColor='secondary'
                          isBold={false}
                        >
                          {getLocale('braveWalletTryDifferentKeywords')}
                        </Text>
                      </Column>
                    ) : (
                      <Column
                        width='100%'
                        gap='16px'
                      >
                        {filteredQuotes?.map((quote, index) => (
                          <BuyQuote
                            key={quote.serviceProvider}
                            quote={quote}
                            serviceProviders={serviceProviders || []}
                            isBestOption={
                              filteredQuotes.length > 1 && index === 0
                            }
                            isOpenOverride={index === 0}
                            isCreatingWidget={
                              isCreatingWidgetFor === quote.serviceProvider
                            }
                            onBuy={onBuy}
                            selectedAsset={selectedAsset}
                          />
                        ))}
                      </Column>
                    )}
                  </>
                )}
              </>
            )}
          </ServiceProvidersWrapper>
        </ContentWrapper>
      </WalletPageWrapper>

      <SelectAsset
        isOpen={isAssetDialogOpen}
        assets={cryptoCurrencies || []}
        selectedAsset={selectedAsset}
        spotPriceRegistry={spotPriceRegistry}
        selectedFiatCurrency={selectedCurrency}
        isLoadingAssets={isLoadingAssets}
        isLoadingSpotPrices={isLoadingSpotPrices}
        onSelectAsset={(asset) => {
          onSelectToken(asset)
          setIsAssetDialogOpen(false)
        }}
        onClose={() => setIsAssetDialogOpen(false)}
      />

      <SelectCurrency
        isOpen={isCurrencyDialogOpen}
        selectedCurrency={selectedCurrency}
        currencies={fiatCurrencies || []}
        onSelectCurrency={(currency) => {
          onSelectCurrency(currency)
          setIsCurrencyDialogOpen(false)
        }}
        onClose={() => setIsCurrencyDialogOpen(false)}
      />

      <SelectAccount
        isOpen={isAccountDialogOpen}
        accounts={accounts}
        onSelect={(account) => {
          onSelectAccount(account)
          setIsAccountDialogOpen(false)
        }}
        selectedAsset={selectedAsset}
        onClose={() => setIsAccountDialogOpen(false)}
      />

      <CreateAccount
        isOpen={showCreateAccount}
        token={pendingSelectedToken || selectedAsset}
        onClose={onCloseCreateAccount}
        onSelectToken={onSelectToken}
      />
    </>
  )
}
