/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledRemove = styled<{}, 'div'>('div')`
  font-family: Muli, sans-serif;
  font-size: 14px;
  line-height: 1;
  margin-top: 3px;
  color: #d1d1db;
`

export const StyledRemoveIcon = styled<{}, 'span'>('span')`
  vertical-align: text-bottom;
  width: 9px;
`

export const StyledType = styled<{}, 'div'>('div')`
  font-family: Muli, sans-serif;
  font-size: 14px;
  font-weight: 600;
  line-height: 1.29;
  color: #686978;
`

export const StyledDate = styled<{}, 'div'>('div')`
  font-family: Muli, sans-serif;
  font-size: 14px;
  line-height: 1;
  margin-top: 3px;
  color: #b8b9c4;
`

export const StyledToggle = styled<{}, 'button'>('button')`
  font-family: Poppins, sans-serif;
  font-size: 13px;
  color: #4c54d2;
  text-transform: capitalize;
  padding: 0;
  border: none;
  background: none;
  cursor: pointer;
`

export const StyledRecurringIcon = styled<{}, 'span'>('span')`
  display: inline-block;
  margin-left: 3px;
  vertical-align: middle;
`

export const StyledToggleWrap = styled<{}, 'div'>('div')`
  text-align: right;
`
