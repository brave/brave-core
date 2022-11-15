/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledRemove = styled('button')<{}>`
  font-family: Poppins, sans-serif;
  font-size: 14px;
  line-height: 0;
  height: 20px;
  color: #9E9FAB;
  padding: 0;
  border: none;
  background: none;
  cursor: pointer;
  display: inline-block;
`

export const StyledRemoveIcon = styled('span')<{}>`
  vertical-align: middle;
  color: #9E9FAB;
  width: 12px;
  height: 12px;
  display: inline-block;
  margin-right: 4px;
`

export const StyledType = styled('div')<{}>`
  font-family: Poppins, sans-serif;
  font-size: 14px;
  font-weight: 600;
  color: #686978;
`

export const StyledDate = styled('div')<{}>`
  font-family: Poppins, sans-serif;
  font-size: 14px;
  line-height: 1;
  margin-top: 3px;
  color: #b8b9c4;
`

export const StyledTipDate = styled(StyledDate)`
  margin-top: 0px;
`

export const StyledToggle = styled('button')<{}>`
  font-family: Poppins, sans-serif;
  font-size: 13px;
  color: #4c54d2;
  padding: 0;
  border: none;
  background: none;
  cursor: pointer;
`

export const StyledToggleWrap = styled('div')<{}>`
  text-align: right;
`

export const StyledLink = styled('a')<{}>`
  text-decoration: none;
`
