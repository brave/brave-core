/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Fuse from 'fuse.js'

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
import {
  SelectScrollSearchContainer
} from '../shared-styles'
import { SelectCurrencyWrapper } from './select-currency.style'
import { VerticalSpace } from '../../shared/style'
import { LoadingRing } from '../../extension/add-suggested-token-panel/style'

export interface Props {
  onSelectCurrency: (currency: BraveWallet.OnRampCurrency) => void
  onBack: () => void
}

const FUSE_CONFIG = {
  shouldSort: true,
  threshold: 0.45,
  location: 0,
  distance: 100,
  minMatchCharLength: 1,
  keys: [
    { name: 'currencyName', weight: 0.5 },
    { name: 'currencyCode', weight: 0.5 }
  ]
}

export const SelectOnRampFiatCurrency = (props: Props) => {
  const { onSelectCurrency, onBack } = props

  // queries
  const { data: currencies, isLoading } = useGetOnRampFiatCurrenciesQuery()

  // state
  const [search, setSearch] = React.useState('')

  // memos
  const fuse = React.useMemo(
    () => (currencies ? new Fuse(currencies, FUSE_CONFIG) : undefined),
    [currencies]
  )

  const filteredCurrencies = React.useMemo(() => {
    return search && fuse
      ? fuse
          .search(search)
          .map(
            (result: Fuse.FuseResult<BraveWallet.OnRampCurrency>) => result.item
          )
      : currencies || []
  }, [search, currencies, fuse])

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
