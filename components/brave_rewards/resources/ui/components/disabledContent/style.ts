/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { Props } from './index'

export const StyledContent = styled('div')<Props>`
  font-size: 16px;
  line-height: 1.7;
  padding: 8px;
`

export const StyledTwoColumn = styled('div')<{}>`
  display: flex;
  align-items: center;
  padding: 0;
  margin: 0 auto 8px;
  max-width: 520px;
`

export const StyledIcon = styled('div')<{}>`
  max-width: 50px;
  padding: 4px;
  margin-right: 20px;
`
