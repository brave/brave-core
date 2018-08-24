/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledWrapper = styled<{}, 'div'>('div')`
  padding: 32px 20px 46px;
  text-align: center;
  font-family: Poppins, sans-serif;
`

export const StyledTitle = styled<{}, 'div'>('div')`
  font-size: 16px;
  font-weight: 500;
  line-height: 1.75;
  letter-spacing: 0.1px;
  text-align: center;
  color: #5bc4fe;
`

export const StyledContent = styled<{}, 'div'>('div')`
  font-family: Muli, sans-serif;
  font-size: 16px;
  line-height: 1.75;
  text-align: center;
  color: #686978;
`
