// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// hooks
import { useBuy } from '../swap/hooks/useBuy'

// utils
import { getLocale } from '../../../../common/locale'
import { getAssetSymbol } from '../../../utils/meld_utils'

// selectors
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../common/selectors'

// components
import WalletPageWrapper from '../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper'
import { PageTitleHeader } from '../../../components/desktop/card-headers/page-title-header'
import { SelectAssetButton } from './components/select_asset_button/select_asset_button'
import { SelectAccountButton } from './components/select_account_button/select_account_button'
import { AmountButton } from './components/amount_button/amount_button'
import { SelectCurrency } from './components/select_currency/select_currency'
import { SelectAccount } from './components/select_account/select_account'
import { SelectAsset } from './components/select_asset/select_asset'
import { BuyQuote } from './components/buy_quote/buy_quote'

// styles
import {
  ContentWrapper,
  ControlPanel,
  Divider,
  Loader,
  LoaderText,
  LoadingWrapper,
  ServiceProvidersWrapper
} from './fund_wallet_v1.style'
import { Column } from '../../../components/shared/style'

export const FundWalletScreen = () => {
  // state
  const [isCurrencyDialogOpen, setIsCurrencyDialogOpen] = React.useState(false)
  const [isAssetDialogOpen, setIsAssetDialogOpen] = React.useState(false)
  const [isAccountDialogOpen, setIsAccountDialogOpen] = React.useState(false)

  // hooks
  const {
    selectedAsset,
    selectedCurrency,
    selectedAccount,
    amount,
    isLoadingAssets,
    isLoadingSpotPrices,
    estimatedCryptoAmount,
    spotPriceRegistry,
    fiatCurrencies,
    accounts,
    cryptoCurrencies,
    defaultFiatCurrency,
    isFetchingQuotes,
    quotes,
    onSelectToken,
    onSelectAccount,
    onSelectCurrency,
    onSetAmount,
    serviceProviders
  } = useBuy()

  // redux
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // computed
  const pageTitle = selectedAsset
    ? `${getLocale('braveWalletBuy')} ${getAssetSymbol(selectedAsset)}`
    : getLocale('braveWalletBuy')

  return (
    <>
      <WalletPageWrapper
        wrapContentInBox={true}
        //   hideNav={isAndroid}
        //   hideHeader={isAndroid}
        cardHeader={<PageTitleHeader title={pageTitle} />}
        hideDivider
        noMinCardHeight
        useDarkBackground={isPanel}
        noPadding={isPanel}
        noCardPadding={isPanel}
      >
        <ContentWrapper>
          <ControlPanel>
            <SelectAssetButton
              labelText='Asset'
              selectedAsset={selectedAsset}
              onClick={() => setIsAssetDialogOpen(true)}
            />
            <SelectAccountButton
              labelText='Account'
              selectedAccount={selectedAccount}
              onClick={() => setIsAccountDialogOpen(true)}
            />
            <AmountButton
              labelText='Amount'
              currencyCode={
                selectedCurrency?.currencyCode || defaultFiatCurrency
              }
              amount={amount}
              onClick={() => {
                setIsCurrencyDialogOpen(true)
              }}
              onChange={onSetAmount}
              estimatedCryptoAmount={estimatedCryptoAmount}
            />
          </ControlPanel>
          <ServiceProvidersWrapper>
            {isFetchingQuotes ? (
              <LoadingWrapper>
                <Loader />
                <LoaderText>Getting best prices...</LoaderText>
              </LoadingWrapper>
            ) : (
              <>
                {quotes?.length > 0 && serviceProviders?.length > 0 ? (
                  <Column width='100%' padding='8px'>
                    <Divider />
                    {quotes?.map((quote) => (
                      <BuyQuote
                        key={quote.serviceProvider}
                        quote={quote}
                        serviceProviders={serviceProviders || []}
                        isBestOption={false}
                      />
                    ))}
                  </Column>
                ) : null}
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
        onClose={() => setIsAccountDialogOpen(false)}
      />
    </>
  )
}
