// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { StyledButton } from '../../shared-swap.styles'

export const Button = styled(StyledButton)<{
  isHeader?: boolean
}>`
  justify-content: space-between;
  padding: 8px 12px;
  white-space: nowrap;
  width: 100%;
  color: ${(p) => p.theme.color.text03};
  &:hover {
    color: ${(p) => p.theme.color.text01};
  }
  @media screen and (max-width: 570px) {
    padding: ${(p) => (p.isHeader ? '8px 20px' : '8px 12px')};
  }
`
