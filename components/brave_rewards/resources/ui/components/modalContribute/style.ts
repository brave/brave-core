/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledWrapper = styled<{}, 'div'>('div')`
  font-family: Poppins, sans-serif;
`

export const StyledTitle = styled<{}, 'div'>('div')`
  font-size: 16px;
  font-weight: 600;
  line-height: 2;
  color: #4b4c5c;
  margin-bottom: 20px;
`

export const StyledContent = styled<{}, 'div'>('div')`
  font-size: 16px;
  color: #4b4c5c;
  margin: 30px 0px 20px;
`

export const StyledNum = styled<{}, 'span'>('span')`
  font-weight: 500;
  color: #0c0d21;
`

export const StyledTabWrapper = styled<{}, 'div'>('div')`
  margin: 0 auto 30px;
`

export const StyledControlWrapper = styled<{}, 'div'>('div')`
  width: 100%;
  margin-bottom: 30px;
`

export const RestoreWrapper = styled<{}, 'div'>('div')`
  float: right;
  margin: -18px 0 -10px;
`
