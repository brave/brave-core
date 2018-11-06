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
`

export const StyledContent = styled<{}, 'div'>('div')`
  font-size: 28px;
  color: #696fdc;
  margin-bottom: 33px;
`

export const StyledNum = styled<{}, 'span'>('span')`
  font-weight: 500;
  color: #0c0d21;
`
