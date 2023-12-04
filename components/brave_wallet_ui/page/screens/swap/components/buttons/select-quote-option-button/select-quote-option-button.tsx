// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../../../../common/locale'
import {
  getTokenPriceAmountFromRegistry //
} from '../../../../../../utils/pricing-utils'
import {
  useGetDefaultFiatCurrencyQuery //
} from '../../../../../../common/slices/api.slice'

// Types
import { QuoteOption } from '../../../constants/types'
import { SpotPriceRegistry } from '../../../../../../constants/types'

// Styled Components
import { BestOptionBadge, Button } from './select-quote-option-button.style'
import { Text, Column } from '../../shared-swap.styles'

interface Props {
  onClick: (option: QuoteOption) => void
  option: QuoteOption
  isSelected: boolean
  isBest: boolean
  spotPrices?: SpotPriceRegistry
}

export const SelectQuoteOptionButton = (props: Props) => {
  const { onClick, option, isSelected, isBest, spotPrices } = props

  // Queries
  const { data: defaultFiatCurrency = 'usd' } = useGetDefaultFiatCurrencyQuery()

  // Methods
  const onSelectToken = React.useCallback(() => {
    onClick(option)
  }, [option, onClick])

  const quoteFiatValue = React.useMemo(() => {
    if (!spotPrices) {
      return ''
    }

    return option.toAmount
      .times(getTokenPriceAmountFromRegistry(spotPrices, option.toToken))
      .formatAsFiat(defaultFiatCurrency)
  }, [spotPrices, option, defaultFiatCurrency])

  // Render
  return (
    <Button
      onClick={onSelectToken}
      isSelected={isSelected}
    >
      {isBest && (
        <BestOptionBadge isSelected={isSelected}>
          {getLocale('braveSwapBest')}
        </BestOptionBadge>
      )}
      <Text
        isBold={true}
        textColor='text01'
        textSize='14px'
        textAlign='left'
      >
        {option.label}
      </Text>
      <Column horizontalAlign='flex-end'>
        <Text
          isBold={true}
          textColor='text01'
          textSize='14px'
          textAlign='right'
        >
          {option.toAmount.formatAsAsset(6, option.toToken.symbol)}
        </Text>
        <Text
          textColor='text03'
          textSize='14px'
          textAlign='right'
        >
          ~{quoteFiatValue}
        </Text>
      </Column>
    </Button>
  )
}
