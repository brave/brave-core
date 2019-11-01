/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'brave-ui/theme'
import { Props } from './index'

export const StyledContent = styled<Props, 'div'>('div')`
  font-size: 16px;
  line-height: 1.7;
  padding: 8px;
`

export const StyledTwoColumn = styled<{}, 'div'>('div')`
  display: flex;
  align-items: center;
  padding: 0 48px;
  margin: 0 0 8px;
`

export const StyledIcon = styled<{}, 'div'>('div')`
  max-width: 50px;
  padding: 4px;
  margin-right: 20px;
`
