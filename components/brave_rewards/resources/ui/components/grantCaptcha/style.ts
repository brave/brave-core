/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledWrapper = styled<{}, 'div'>('div')`
  text-align: center;
  width: 100%;
  margin: 28px 0 0;
`

export const StyledDropArea = styled<{}, 'img'>('img')`
  width: 333px;
  height: 296px;
  margin-left: -32px;
  margin-top: 20px;
`

export const StyledDrag = styled<{}, 'div'>('div')`
  display: flex;
  justify-content: center;
`

export const StyledImageWrap = styled<{}, 'div'>('div')`
  flex-basis: 80px;
  flex-shrink: 0;
  margin-top: -15px;
`

export const StyledText = styled<{}, 'div'>('div')`
  flex-basis: 130px;
  font-family: Muli, sans-serif;
  font-size: 14px;
  line-height: 1.29;
  color: #686978;
  text-align: left;
  padding-left: 13px;
`
