// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// styles
import { Column, Row } from '../../../../../components/shared/style'
import {
  CaretDown,
  ControlsWrapper,
  Label,
  WrapperButton
} from '../shared/style'
import {
  AmountEstimate,
  AmountInput,
  AmountWrapper,
  CurrencyCode,
  LabelWrapper,
  SwapVerticalIcon
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
  // methods
  const onInputChange = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      onChange(event.target.value)
    },
    [onChange]
  )

  return (
    <Column alignItems='flex-start'>
      <Column alignItems='flex-start'>
        <LabelWrapper>
          <Label>{labelText}</Label>
        </LabelWrapper>
        <ControlsWrapper>
          <AmountWrapper>
            <Row>
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
                <Row gap='8px'>
                  <CurrencyCode>{currencyCode}</CurrencyCode>
                  <CaretDown />
                </Row>
              </WrapperButton>
            </Row>
            {estimatedCryptoAmount ? (
              <Row
                justifyContent='flex-end'
                gap='8px'
              >
                <AmountEstimate>{estimatedCryptoAmount}</AmountEstimate>
                <SwapVerticalIcon />
              </Row>
            ) : null}
          </AmountWrapper>
        </ControlsWrapper>
      </Column>
    </Column>
  )
}
