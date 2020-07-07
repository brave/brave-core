/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'brave-ui/theme'
import palette from 'brave-ui/theme/colors'

export const Header = styled<{}, 'div'>('div')`
  text-align: left;
`

export const StyledTitle = styled<{}, 'div'>('div')`
  margin-top: 6px;
  display: flex;
  justify-content: flex-start;
  align-items: center;
  font-size: 18px;
  font-weight: 600;
  color: ${palette.white};
  font-family: ${p => p.theme.fontFamily.heading};
`

export const StyledAddIcon = styled<{}, 'div'>('div')`
  width: 24px;
  height: 24px;
  margin-right: 7px;
  margin-left: 5px;
  margin-top: 2px;
`
