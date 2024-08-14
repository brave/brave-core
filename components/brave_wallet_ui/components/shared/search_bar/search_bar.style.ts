// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'
import Input, { InputProps } from '@brave/leo/react/input'

export type SearchInputProps = Omit<InputProps, 'onChange' | 'onInput'> & {
  margin?: string
}

export const SearchInput = styled(Input)<SearchInputProps>`
  flex: 1;
  margin: ${(p) => p.margin || 'unset'};
`

export const SearchIcon = styled(Icon).attrs({ name: 'search' })`
  --leo-icon-size: 20px;
  color: ${leo.color.icon.default};
`
