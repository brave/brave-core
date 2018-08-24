/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledText = styled<{}, 'div'>('div')`
  font-family: Muli, sans-serif;
  font-size: 14px;
  line-height: 1.29;
  text-align: right;
  color: #686978;
`

export const StyledRemove = styled<{}, 'button'>('button')`
  margin-left: 10px;
  background: none;
  border: none;
  padding: 0 10px 0 0;
  cursor: pointer;
`

export const StyledTHOther = styled<{}, 'div'>('div')`
  text-align: right;
`

export const StyledTHLast = styled(StyledTHOther)`
  padding-right: 10px;
`

export const StyledToggleWrap = styled<{}, 'div'>('div')`
  text-align: right;
`

export const StyledToggle = styled<{}, 'button'>('button')`
  font-family: Poppins, sans-serif;
  font-size: 13px;
  color: #4c54d2;
  text-transform: capitalize;
  background: none;
  border: none;
  padding: 0;
  cursor: pointer;
`
