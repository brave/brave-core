// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { StyledButton } from '../../shared-swap.styles'

export const Button = styled(StyledButton)`
  /* These RGBA values do not exist in the design system */
  --background-color: rgba(255, 255, 255, 0.6);
  --background-color-hover: rgba(218, 220, 232, 0.4);
  @media (prefers-color-scheme: dark) {
  /* These RGBA values do not exist in the design system */
  --background-color: rgba(52, 58, 64, 0.6);
  --background-color-hover: rgba(66, 69, 82, 0.8);
  }
  background-color: var(--background-color);
  border-radius: 4px;
  font-size: 14px;
  margin-right: 8px;
  padding: 1px 6px;
  text-transform: uppercase;
  &:hover:not([disabled]) {
    background-color: var(--background-color-hover);
  }
`
