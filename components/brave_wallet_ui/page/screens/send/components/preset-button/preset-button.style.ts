// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// Shared Styles
import { StyledButton } from '../../shared.styles'

export const Button = styled(StyledButton)`
  --button-background: ${(p) => p.theme.color.background01};
  /* rgba(218, 220, 232, 0.4) does not exist in the design system */
  --button-background-hover: rgba(218, 220, 232, 0.4);
  @media (prefers-color-scheme: dark) {
    --button-background: ${(p) => p.theme.color.background02};
    --button-background-hover: ${(p) => p.theme.color.background01};
    }
  background-color: var(--button-background);
  border-radius: 4px;
  font-size: 14px;
  margin-right: 8px;
  padding: 1px 6px;
  text-transform: uppercase;
  &:hover {
    background-color: var(--button-background-hover);
  }
`
