// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import { Input, SearchIconStyle } from './search-input.style'

interface Props {
  onChange: (value: string) => void
  value: string
  autoFocus?: boolean
  placeholder?: string
}

export const SearchInput = (props: Props) => {
  const { onChange, value, autoFocus, placeholder } = props

  const onInputChange = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      onChange(event.target.value)
    },
    [onChange]
  )

  return (
    <>
      <SearchIconStyle
        name='search'
        size={22}
      />
      <Input
        placeholder={placeholder}
        spellCheck={false}
        autoFocus={autoFocus}
        value={value}
        onChange={onInputChange}
      />
    </>
  )
}
