// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as leo from '@brave/leo/tokens/css/variables'
import styled from 'styled-components'
import { Column } from '../../shared-swap.styles'

export const Section = styled(Column)`
  background-color: ${leo.color.page.background};
  border: 1px solid ${leo.color.divider.subtle};
  border-radius: 8px;
`

export const Link = styled.a`
  font: ${leo.font.default.regular};
  color: ${leo.color.button.background};
  text-decoration: none;
  display: block;
  cursor: pointer;
`
