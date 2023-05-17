// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import { StyledInput } from '../../shared-swap.styles'

export const Input = styled(StyledInput) <{
  hasError: boolean
}>`
  color: ${(p) => (p.hasError ? leo.color.red[40] : 'inherit')};
  font-weight: 500;
  font-size: 28px;
  line-height: 42px;
  text-align: right;
  width: 100%;
  ::placeholder {
    color: ${(p) => p.theme.color.text03};
  }
`
