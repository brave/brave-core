// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// Shared Styles
import { StyledButton } from '../../shared.styles'

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

export const AccountCircle = styled.div<{
  orb: string
}>`
  width: 32px;
  height: 32px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
  margin-right: 16px;
`
