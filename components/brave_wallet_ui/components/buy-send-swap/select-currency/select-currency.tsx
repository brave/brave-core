/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import Fuse from 'fuse.js'

import { BraveWallet, WalletState } from '../../../constants/types'
import { SearchBar } from '../../shared'
import Header from '../select-header'
import { getLocale } from '../../../../common/locale'

// Components
import { SelectCurrencyItem } from '../select-currency-item/select-currency-item'

// Styled Components
import {
  SelectScrollSearchContainer
} from '../shared-styles'
import { SelectCurrencyWrapper } from './select-currency.style'
import { WalletActions } from '../../../common/actions'

export interface Props {
  onSelectCurrency?: (currency: BraveWallet.OnRampCurrency) => void
  onBack: () => void
}

export const SelectCurrency = (props: Props) => {
  const { onSelectCurrency, onBack } = props
  const {
    onRampCurrencies: currencies
  } = useSelector((state: { wallet: WalletState }) => state.wallet)

  // redux
  const dispatch = useDispatch()

  const fuse = React.useMemo(() => new Fuse(currencies, {
    shouldSort: true,
    threshold: 0.45,
    location: 0,
    distance: 100,
    minMatchCharLength: 1,
    keys: [
      { name: 'currencyName', weight: 0.5 },
      { name: 'currencyCode', weight: 0.5 }
    ]
  }), [currencies])

  const [filteredCurrencies, setFilteredCurrencies] = React.useState<BraveWallet.OnRampCurrency[]>(currencies)

  // methods
  const filterCurrencyList = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    const search = event.target.value
    if (search === '') {
      setFilteredCurrencies(currencies)
    } else {
      const filteredList = fuse.search(search).map((result: Fuse.FuseResult<BraveWallet.OnRampCurrency>) => result.item)
      setFilteredCurrencies(filteredList)
    }
  }, [fuse, currencies])

  const onSelectedCurrency = React.useCallback((currency: BraveWallet.OnRampCurrency) => {
    dispatch(WalletActions.selectCurrency(currency))

    if (onSelectCurrency) {
      onSelectCurrency(currency)
    }
  }, [onSelectCurrency])

  // effects
  React.useEffect(() => {
    if (filteredCurrencies.length === 0 && currencies.length > 0) {
      setFilteredCurrencies(currencies)
      dispatch(WalletActions.selectCurrency(currencies[0]))
    }
  }, [currencies, filteredCurrencies])

  return (
    <SelectCurrencyWrapper>
      <Header
        title={getLocale('braveWalletSelectCurrency')}
        onBack={onBack}
        hasAddButton={false}
      />
      <SearchBar placeholder={getLocale('braveWalletSearchCurrency')} action={filterCurrencyList} autoFocus={true} />
      <SelectScrollSearchContainer>
        {
          filteredCurrencies.map((currency: BraveWallet.OnRampCurrency) =>
            <SelectCurrencyItem
              key={currency.currencyCode}
              currency={currency}
              onSelectCurrency={onSelectedCurrency}
            />
          )
        }
      </SelectScrollSearchContainer>
    </SelectCurrencyWrapper>
  )
}
