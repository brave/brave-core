/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import palette from '../../../theme/colors'

export const StyledRemove = styled<{}, 'button'>('button')`
  padding: 0;
  border: none;
  background: none;
  cursor: pointer;
  text-align: center;
  width: 100%;
`

export const StyledRemoveIcon = styled<{}, 'span'>('span')`
  vertical-align: middle;
  color: ${palette.grey400};
  width: 20px;
  height: 20px;
  display: inline-block;
  margin-right: 4px;
`
export const StyledLink = styled<{}, 'a'>('a')`
  text-decoration: none;
`
