// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import { Input } from './standard-input.style'

interface Props {
  onChange: (value: string) => void
  value: string
  autoFocus?: boolean
  placeholder?: string
  disabled?: boolean
}

export const StandardInput = (props: Props) => {
  const { onChange, value, autoFocus, placeholder, disabled } = props

  const onInputChange = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      onChange(event.target.value)
    },
    [onChange]
  )

  return (
    <Input
      placeholder={placeholder}
      spellCheck={false}
      autoFocus={autoFocus}
      value={value}
      onChange={onInputChange}
      disabled={disabled}
    />
  )
}
