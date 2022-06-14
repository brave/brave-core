/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { BraveWallet } from '../../../constants/types'

// Styled Components
import {
  StyledWrapper,
  CurrencySymbol,
  CurrencyName,
  CurrencyNameAndCode
} from './select-currency-item-styles'
import { IconWrapper, PlaceholderText } from '../../shared/create-placeholder-icon/style'

// utils
import { background } from 'ethereum-blockies'
import { CurrencySymbols } from '../../../utils/currency-symbols'

export interface Props {
  currency: BraveWallet.OnRampCurrency
  onSelectCurrency: (currency: BraveWallet.OnRampCurrency) => void
}

export const SelectCurrencyItem = (props: Props) => {
  const { currency, onSelectCurrency } = props

  // methods
  const onClick = React.useCallback(() => {
    onSelectCurrency(currency)
  }, [currency, onSelectCurrency])

  // memos
  const bg = React.useMemo(() => {
    return background({ seed: window.btoa(currency.currencyName + currency.currencyCode) })
  }, [currency])

  return (
    <StyledWrapper onClick={onClick}>
      <IconWrapper
        panelBackground={bg}
        isPlaceholder={true}
        size='small'
        marginLeft={0}
        marginRight={8}
      >
        <PlaceholderText size='small'>{CurrencySymbols[currency.currencyCode]}</PlaceholderText>
      </IconWrapper>
      <CurrencyNameAndCode>
        <CurrencyName>{currency.currencyName}</CurrencyName>
        <CurrencySymbol>{currency.currencyCode}</CurrencySymbol>
      </CurrencyNameAndCode>
    </StyledWrapper>
  )
}
