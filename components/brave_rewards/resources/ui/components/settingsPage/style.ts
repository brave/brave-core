/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledWrapper = styled<{}, 'div'>('div')`
  background: #f2f4f7;
  min-height: 100vh;
  width: 100%;
  font-family: "Poppins", sans-serif
`

export const StyleHeader = styled<{}, 'div'>('div')`
  background-image: linear-gradient(267deg, #bf14a2, #f73a1c);
  height: 62px;
`

export const StyledContent = styled<{}, 'div'>('div')`
 max-width: 1000px;
 margin: 0 auto;
 padding: 40px 0;
`
