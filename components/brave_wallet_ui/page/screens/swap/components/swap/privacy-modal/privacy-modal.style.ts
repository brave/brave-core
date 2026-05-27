// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import { Column } from '../../shared-swap.styles'

export const Section = styled(Column)`
  background-color: ${(p) => p.theme.color.background01};
  border: 1px solid ${(p) => p.theme.color.divider01};
  border-radius: 8px;
`

export const Link = styled.a`
  font: ${leo.font.default.regular};
  color: ${(p) => p.theme.color.interactive05};
  text-decoration: none;
  display: block;
  cursor: pointer;
  @media (prefers-color-scheme: dark) {
    color: ${(p) => p.theme.color.interactive06};
  }
`
