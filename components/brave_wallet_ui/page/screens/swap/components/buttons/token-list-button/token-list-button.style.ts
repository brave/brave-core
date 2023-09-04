// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { StyledButton, Column } from '../../shared-swap.styles'

export const Button = styled(StyledButton)`
  --button-shadow: 0px 0px 10px rgba(79, 79, 79, 0.1);
  @media (prefers-color-scheme: dark) {
    --button-shadow: 0px 0px 16px rgba(0, 0, 0, 0.36);
  }
  background-color: none;
  border-radius: 8px;
  justify-content: space-between;
  padding: 16px 8px;
  white-space: nowrap;
  width: calc(100% - 24px);
  margin-left: 12px;
  :disabled {
    opacity: 0.3;
  }
  &:hover:not([disabled]) {
    box-shadow: var(--button-shadow);
    background-color: ${(p) => p.theme.color.background01};
  }
`

export const NameAndSymbol = styled(Column)`
  @media screen and (max-width: 570px) {
    width: 40vw;
    overflow: hidden;
    white-space: pre-line;
  }
`
