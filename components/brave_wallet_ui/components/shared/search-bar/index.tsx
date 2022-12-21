// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import {
  StyledWrapper,
  SearchInput,
  SearchIcon
} from './style'

interface Props {
  placeholder: string
  action?: React.ChangeEventHandler<HTMLInputElement> | undefined
  autoFocus?: boolean
  value?: string
  useWithFilter?: boolean
  disabled?: boolean
}

export const SearchBar = (props: Props) => {
  const { autoFocus, placeholder, action, value, useWithFilter, disabled } = props
  return (
    <StyledWrapper
      useWithFilter={useWithFilter}
    >
      <SearchIcon />
      <SearchInput
        autoFocus={autoFocus}
        value={value}
        placeholder={placeholder}
        onChange={action}
        disabled={disabled}
      />
    </StyledWrapper>
  )
}

export default SearchBar
