// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { DialogProps } from '@brave/leo/react/dialog'
import Icon from '@brave/leo/react/icon'

// Selectors
import {
  useSafeUISelector //
} from '../../../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../../../common/selectors'

// Types
import { MeldFiatCurrency } from '../../../../../constants/types'

// Utils
import { getLocale } from '../../../../../../common/locale'

// Components
import {
  BottomSheet //
} from '../../../../../components/shared/bottom_sheet/bottom_sheet'

// Styled Components
import {
  Column,
  Row,
  ScrollableColumn
} from '../../../../../components/shared/style'
import {
  CurrencyImage,
  CurrencyName,
  CurrencyCode,
  SearchInput,
  SelectedLabel
} from './select_currency.style'
import { ContainerButton, Dialog, DialogTitle } from '../shared/style'

interface SelectCurrencyProps extends DialogProps {
  currencies: MeldFiatCurrency[]
  selectedCurrency?: MeldFiatCurrency
  isOpen: boolean
  onClose: () => void
  onSelectCurrency: (currency: MeldFiatCurrency) => void
}

export const CurrencyListItem = ({
  currency,
  isSelected,
  onSelect
}: {
  currency: MeldFiatCurrency
  isSelected?: boolean
  onSelect: (currency: MeldFiatCurrency) => void
}) => {
  return (
    <ContainerButton onClick={() => onSelect(currency)}>
      <Row
        justifyContent='flex-start'
        gap='16px'
      >
        <CurrencyImage src={`chrome://image?${currency.symbolImageUrl}`} />
        <CurrencyName>{currency.name}</CurrencyName>
      </Row>
      <Row
        justifyContent='flex-end'
        gap='16px'
      >
        {isSelected && (
          <SelectedLabel>{getLocale('braveWalletSelected')}</SelectedLabel>
        )}
        <CurrencyCode>{currency.currencyCode}</CurrencyCode>
      </Row>
    </ContainerButton>
  )
}

export const SelectCurrency = (props: SelectCurrencyProps) => {
  const {
    currencies,
    selectedCurrency,
    onSelectCurrency,
    isOpen,
    onClose,
    ...rest
  } = props

  // Selectors
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // State
  const [searchText, setSearchText] = React.useState('')

  // Memos
  const searchResults = React.useMemo(() => {
    if (searchText === '') return currencies

    return currencies.filter((currency) => {
      return (
        currency?.name?.toLowerCase().includes(searchText.toLowerCase()) ||
        currency.currencyCode.toLowerCase().includes(searchText.toLowerCase())
      )
    })
  }, [currencies, searchText])

  const selectCurrencyContent = React.useMemo(() => {
    return (
      <>
        <DialogTitle slot='title'>
          {getLocale('braveWalletSelectCurrency')}
        </DialogTitle>
        <Row
          padding='24px 0 0 0'
          slot='subtitle'
        >
          <SearchInput
            placeholder={getLocale('braveWalletSearchCurrency')}
            onInput={(e) => setSearchText(e.value)}
          >
            <Icon
              name='search'
              slot='left-icon'
            />
          </SearchInput>
        </Row>
        <Column
          width='100%'
          height='80vh'
          justifyContent='flex-start'
        >
          {searchResults.length === 0 ? (
            <Row
              justifyContent='center'
              alignItems='center'
            >
              {getLocale('braveWalletNoAvailableCurrencies')}
            </Row>
          ) : (
            <ScrollableColumn>
              {searchResults.map((currency) => (
                <CurrencyListItem
                  key={currency.currencyCode}
                  currency={currency}
                  onSelect={onSelectCurrency}
                  isSelected={
                    currency.currencyCode === selectedCurrency?.currencyCode
                  }
                />
              ))}
            </ScrollableColumn>
          )}
        </Column>
      </>
    )
  }, [onSelectCurrency, searchResults, selectedCurrency])

  if (isPanel) {
    return (
      <BottomSheet
        onClose={onClose}
        isOpen={isOpen}
      >
        <Column
          fullWidth={true}
          padding='0px 16px'
          height='90vh'
        >
          {selectCurrencyContent}
        </Column>
      </BottomSheet>
    )
  }

  return (
    <Dialog
      {...rest}
      isOpen={isOpen}
      onClose={onClose}
      showClose
      size='mobile'
    >
      {selectCurrencyContent}
    </Dialog>
  )
}
