// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import { Input } from './swap-input.style'

interface Props {
  onChange: (value: string) => void
  hasError: boolean
  value: string
  autoFocus?: boolean
  disabled?: boolean
}

export const SwapInput = (props: Props) => {
  const { onChange, value, autoFocus, hasError, disabled } = props

  const onInputChange = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      onChange(event.target.value)
    },
    [onChange]
  )

  return (
    <Input
      placeholder='0.0'
      type='number'
      spellCheck={false}
      autoFocus={autoFocus}
      value={value}
      hasError={hasError}
      onChange={onInputChange}
      disabled={disabled ?? false}
    />
  )
}
