/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import palette from '../../../theme/colors'
import styled from 'styled-components'

export const StyledMediaBox = styled<{}, 'div'>('div')`
  border: 1px solid ${palette.grey400};
  border-radius: 5px;
  margin: 15px 0 0 0;
  padding: 15px;
  text-overflow: ellipsis;
  white-space: pre-wrap;
  overflow: hidden;
`

export const StyledMediaTimestamp = styled<{}, 'div'>('div')`
  color: ${palette.grey500};
  font-size: 12px;
  display: inline;
  vertical-align: middle;
`

export const StyledMediaText = styled<{}, 'p'>('p')`
  font-size: 14px;
`

export const StyledMediaIcon = styled<{}, 'span'>('span')`
  width: 22px;
  height: 22px;
  margin-right: 5px;
  display: inline-block;
  vertical-align: middle;
`
