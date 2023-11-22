/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import { getLocale } from '../../../../common/locale'

// Components
import { SearchBar } from '../../shared/search-bar/index'
import Header from '../select-header'
import { SelectCurrencyItem } from '../select-currency-item/select-currency-item'

// Hooks
import {
  useGetOnRampFiatCurrenciesQuery //
} from '../../../common/slices/api.slice'

// Styled Components
import { SelectScrollSearchContainer } from '../shared-styles'
import { SelectCurrencyWrapper } from './select-currency.style'
import { VerticalSpace } from '../../shared/style'
import { LoadingRing } from '../../extension/add-suggested-token-panel/style'

export interface Props {
  onSelectCurrency: (currency: BraveWallet.OnRampCurrency) => void
  onBack: () => void
}

export const SelectOnRampFiatCurrency = (props: Props) => {
  const { onSelectCurrency, onBack } = props

  // queries
  const { data: currencies, isLoading } = useGetOnRampFiatCurrenciesQuery()

  // state
  const [search, setSearch] = React.useState('')

  // memos
  const filteredCurrencies = React.useMemo(() => {
    const trimmedSearch = search.trim().toLowerCase()
    return currencies && trimmedSearch
      ? currencies.filter(
          (c) =>
            c.currencyCode.toLowerCase().includes(trimmedSearch) ||
            c.currencyName.toLowerCase().includes(trimmedSearch)
        )
      : currencies || []
  }, [search, currencies])

  return (
    <SelectCurrencyWrapper>
      <Header
        title={getLocale('braveWalletSelectCurrency')}
        onBack={onBack}
        hasAddButton={false}
      />
      <SearchBar
        placeholder={getLocale('braveWalletSearchCurrency')}
        action={(e) => setSearch(e.target.value)}
        autoFocus={true}
        isV2={true}
      />
      <VerticalSpace space='16px' />
      <SelectScrollSearchContainer>
        {isLoading ? (
          <LoadingRing />
        ) : (
          filteredCurrencies.map((currency: BraveWallet.OnRampCurrency) => (
            <SelectCurrencyItem
              key={currency.currencyCode}
              currency={currency}
              onSelectCurrency={onSelectCurrency}
            />
          ))
        )}
      </SelectScrollSearchContainer>
    </SelectCurrencyWrapper>
  )
}
