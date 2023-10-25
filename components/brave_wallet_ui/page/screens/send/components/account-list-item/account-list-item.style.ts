// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// Shared Styles
import { StyledButton, Text } from '../../shared.styles'

export const Button = styled(StyledButton)`
  --button-shadow-hover: 0px 0px 16px rgba(99, 105, 110, 0.18);
  @media (prefers-color-scheme: dark) {
    --button-shadow-hover: 0px 0px 16px rgba(0, 0, 0, 0.36);
  }
  width: 100%;
  border-radius: 10px;
  padding: 10px 12px;
  justify-content: flex-start;
  align-items: center;
  flex-direction: row;
  &:disabled {
    opacity: 0.4;
  }
  &:hover:not([disabled]) {
    box-shadow: var(--button-shadow-hover);
  }
`

export const ButtonText = styled(Text)`
  word-break: break-all;
`
