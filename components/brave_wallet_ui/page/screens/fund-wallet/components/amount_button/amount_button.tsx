// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import { Row } from '../../../../../components/shared/style'
import { CaretDown, Label, WrapperButton } from '../shared/style'
import {
  AmountEstimate,
  AmountInput,
  CurrencyCode,
  SwapVerticalIcon,
  Wrapper,
  ButtonWrapper
} from './amount_button.style'

interface SelectAccountProps {
  labelText: string
  amount?: string
  currencyCode?: string
  estimatedCryptoAmount?: string
  onChange: (amount: string) => void
  onClick: () => void
}

export const AmountButton = ({
  labelText,
  currencyCode,
  amount,
  estimatedCryptoAmount,
  onChange,
  onClick
}: SelectAccountProps) => {
  // Methods
  const onInputChange = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      onChange(event.target.value)
    },
    [onChange]
  )

  return (
    <Wrapper>
      <Label>{labelText}</Label>
      <ButtonWrapper>
        <Row
          width='unset'
          margin='0px 0px 12px 0px'
        >
          <AmountInput
            placeholder='0.0'
            type='number'
            spellCheck={false}
            onChange={onInputChange}
            value={amount}
            hasError={false}
            autoFocus={true}
          />
          <WrapperButton onClick={onClick}>
            <Row
              gap='8px'
              width='unset'
            >
              <CurrencyCode>{currencyCode}</CurrencyCode>
              <CaretDown />
            </Row>
          </WrapperButton>
        </Row>
        {estimatedCryptoAmount ? (
          <Row
            width='unset'
            justifyContent='flex-end'
            gap='4px'
          >
            <AmountEstimate>{estimatedCryptoAmount}</AmountEstimate>
            <SwapVerticalIcon />
          </Row>
        ) : null}
      </ButtonWrapper>
    </Wrapper>
  )
}
