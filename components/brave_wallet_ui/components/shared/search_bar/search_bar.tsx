// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import {
  SearchIcon,
  SearchInput,
  type SearchInputProps
} from './search_bar.style'

type Props = SearchInputProps & {
  onChange: (value: string) => void
  autoFocus?: boolean
}

export const SearchBar = ({ autoFocus, onChange, ...rest }: Props) => {
  // refs
  const ref = React.useRef<HTMLInputElement>(null)

  // effects
  React.useEffect(() => {
    if (autoFocus) {
      ref.current?.focus()
    }
  }, [autoFocus])

  // render
  return (
    <SearchInput
      ref={ref}
      {...rest}
      onInput={(eventDetail) => onChange(eventDetail.value)}
    >
      <SearchIcon slot='left-icon' />
    </SearchInput>
  )
}
