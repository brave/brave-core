/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'brave-ui/theme'

export const StyledWrapper = styled<{}, 'div'>('div')`
  background: #f2f4f7;
  min-height: 100vh;
  min-width: 1024px;
  font-family: ${p => p.theme.fontFamily.body};
`

export const StyledContent = styled<{}, 'div'>('div')`
 max-width: 1024px;
 margin: 0 auto;
 padding: 48px 0;
`
