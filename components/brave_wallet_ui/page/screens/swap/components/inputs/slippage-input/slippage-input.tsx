// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import {
  CustomSlippageInput,
  InputWrapper
} from './slippage-input.style'
import { Text } from '../../shared-swap.styles'

interface Props {
  onChange: (value: string) => void
  value: string
}

export const SlippageInput = (props: Props) => {
  const { onChange, value } = props

  // Methods
  const onInputChange = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      onChange(event.target.value)
    },
    [onChange]
  )

  return (
    <InputWrapper>
      <CustomSlippageInput
        type='number'
        value={value}
        onChange={onInputChange}
        placeholder='0'
      />
      <Text textSize='14px' textColor='text03' isBold={true}>
        %
      </Text>
    </InputWrapper>
  )
}
