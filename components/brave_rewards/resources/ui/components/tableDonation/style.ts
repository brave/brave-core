/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledRemove = styled<{}, 'button'>('button')`
  font-family: Muli, sans-serif;
  font-size: 14px;
  line-height: 1;
  margin-top: 4px;
  color: #9E9FAB;
  padding: 0;
  border: none;
  background: none;
  cursor: pointer;
`

export const StyledRemoveIcon = styled<{}, 'span'>('span')`
  vertical-align: middle;
  color: #9E9FAB;
  width: 12px;
  height: 12px;
  display: inline-block;
  margin-right: 4px;
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

export const StyledToggleWrap = styled<{}, 'div'>('div')`
  text-align: right;
`

export const StyledLink = styled<{}, 'a'>('a')`
  text-decoration: none;
`
