// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { QuoteOption } from '../../../constants/types'
import { SpotPriceRegistry } from '../../../../../../constants/types'

// Components
import {
  SelectQuoteOptionButton //
} from '../../buttons/select-quote-option-button/select-quote-option-button'

// Styled Components
import { MoreOptionsButton } from './quote-options.style'
import { VerticalSpacer, Column, Icon } from '../../shared-swap.styles'

interface Props {
  options: QuoteOption[]
  selectedQuoteOptionIndex: number
  onSelectQuoteOption: (index: number) => void
  spotPrices?: SpotPriceRegistry
}

export const QuoteOptions = (props: Props) => {
  const { options, selectedQuoteOptionIndex, onSelectQuoteOption, spotPrices } =
    props

  // State
  const [showAllOptions, setShowAllOptions] = React.useState<boolean>(false)

  // Methods
  const onToggleShowAllOptions = React.useCallback(() => {
    setShowAllOptions((prev) => !prev)
  }, [])

  // Memos
  const filteredQuoteOptions: QuoteOption[] = React.useMemo(() => {
    if (showAllOptions) {
      return options
    }
    return options.slice(0, 2)
  }, [options, showAllOptions])

  return (
    <>
      <VerticalSpacer size={15} />
      <Column
        columnHeight='dynamic'
        columnWidth='full'
      >
        {filteredQuoteOptions.map((option: QuoteOption, index) => (
          <SelectQuoteOptionButton
            isBest={index === 0}
            isSelected={selectedQuoteOptionIndex === index}
            onClick={() => onSelectQuoteOption(index)}
            option={option}
            spotPrices={spotPrices}
            key={index}
          />
        ))}
      </Column>
      <MoreOptionsButton
        isSelected={showAllOptions}
        onClick={onToggleShowAllOptions}
      >
        <Icon name='carat-down' />
      </MoreOptionsButton>
    </>
  )
}
