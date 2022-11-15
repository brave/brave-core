/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import palette from 'brave-ui/theme/colors'

export const StyledRemove = styled('button')<{}>`
  padding: 0;
  border: none;
  background: none;
  cursor: pointer;
  text-align: center;
  width: 100%;
`

export const StyledRemoveIcon = styled('span')<{}>`
  vertical-align: middle;
  color: ${palette.grey400};
  width: 20px;
  height: 20px;
  display: inline-block;
  margin-right: 4px;
`
export const StyledLink = styled('a')<{}>`
  text-decoration: none;
`
