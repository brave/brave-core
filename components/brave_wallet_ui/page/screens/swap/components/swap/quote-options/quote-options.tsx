// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import {
  QuoteOption
} from '../../../../../../constants/types'

// Components
import {
  SelectQuoteOptionButton
} from '../../buttons/select-quote-option-button/select-quote-option-button'

// Assets
// FIXME(douglashdaniel): This is not the correct icon
import CaratDownIcon from '../../../assets/lp-icons/0x.svg'

// Styled Components
import {
  MoreOptionsButton
} from './quote-options.style'
import {
  VerticalSpacer,
  Column
} from '../../shared-swap.styles'

// Utils
// import Amount from '../../../../../../utils/amount'

interface Props {
  options: QuoteOption[]
  selectedQuoteOptionIndex: number
  onSelectQuoteOption: (index: number) => void
}

export const QuoteOptions = (props: Props) => {
  const { options, selectedQuoteOptionIndex, onSelectQuoteOption } = props

  // Context
  // const { getTokenPrice } = useSwapContext()

  // State
  const [showAllOptions, setShowAllOptions] = React.useState<boolean>(false)
  const [
    spotPrice,
    // setSpotPrice
  ] = React.useState<number | undefined>(
    undefined
  )

  // Effects
  // React.useEffect(() => {
  //   let ignore = false
  //   if (options[selectedQuoteOptionIndex] !== undefined) {
  //     getTokenPrice(options[selectedQuoteOptionIndex].toToken)
  //       .then((result) => {
  //         if (!ignore) {
  //           setSpotPrice(Number(Amount.normalize(result)))
  //         }
  //       })
  //       .catch((error) => console.log(error))
  //     return () => {
  //       ignore = true
  //     }
  //   }
  // }, [options, selectedQuoteOptionIndex, getTokenPrice])

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
      <Column columnHeight='dynamic' columnWidth='full'>
        {filteredQuoteOptions.map((option: QuoteOption, index) => (
          <SelectQuoteOptionButton
            isBest={index === 0}
            isSelected={selectedQuoteOptionIndex === index}
            onClick={() => onSelectQuoteOption(index)}
            option={option}
            spotPrice={spotPrice}
            key={index}
          />
        ))}
      </Column>
      <MoreOptionsButton
        isSelected={showAllOptions}
        icon={CaratDownIcon}
        onClick={onToggleShowAllOptions}
      />
    </>
  )
}
