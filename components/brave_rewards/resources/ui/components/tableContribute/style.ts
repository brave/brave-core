/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledText = styled('div')<{}>`
  font-family: Poppins, sans-serif;
  font-size: 14px;
  text-align: right;
  color: #686978;
`

export const StyledRemove = styled('button')<{}>`
  margin: 4px 8px 0;
  background: none;
  border: none;
  cursor: pointer;
  width: 16px;
  height: 16px;
  color: #9E9FAB;
  padding: 0;
`

export const StyledTHOther = styled('div')<{}>`
  text-align: right;
`

export const StyledTHLast = styled(StyledTHOther)`
  padding-right: 10px;
`

export const StyledToggleWrap = styled('div')<{}>`
  text-align: right;
`

export const StyledToggle = styled('button')<{}>`
  font-family: Poppins, sans-serif;
  font-size: 13px;
  color: #4c54d2;
  text-transform: capitalize;
  background: none;
  border: none;
  padding: 0;
  cursor: pointer;
`

export const StyledLink = styled('a')<{}>`
  text-decoration: none;
`
