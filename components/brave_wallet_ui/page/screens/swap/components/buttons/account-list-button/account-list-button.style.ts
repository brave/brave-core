// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import { StyledButton } from '../../shared-swap.styles'

export const Button = styled(StyledButton)`
  --background-color-hover: ${leo.color.purple[10]};
  @media (prefers-color-scheme: dark) {
    --background-color-hover: ${(p) => p.theme.color.background02};
  }
  border-radius: 4px;
  justify-content: space-between;
  padding: 5px 18px;
  white-space: nowrap;
  width: 100%;
  background-color: none;
  margin-bottom: 2px;
  &:hover {
    background-color: var(--background-color-hover);
  }
`
