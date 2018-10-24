/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledWrapper = styled<{}, 'div'>('div')`
  text-align: center;
  width: 100%;
  padding: 20px 10px;
  font-family: Poppins, sans-serif;
  min-height: 350px;
`

export const StyledText = styled<{}, 'div'>('div')`
  font-family: Muli, sans-serif;
  font-size: 14px;
  line-height: 1.29;
  color: #838391;
  margin: 44px 0 32px;
`

export const StyledButton = styled<{}, 'div'>('div')`
  display: block;
  margin: 0 auto;
`
