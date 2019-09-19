/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'brave-ui/theme'

export const StyledWrapper = styled<{}, 'div'>('div')`
  position: relative;
  display: flex;
  border-bottom: solid 1px #E5E5EA;
  justify-content: space-between;
  font-family: ${p => p.theme.fontFamily.body};
  align-items: center;
`

export const StyledTitle = styled<{}, 'div'>('div')`
  font-size: 16px;
  color: #4b4c5c;
  padding: 16px 0;
  font-weight: 600;
`

export const StyledContentWrapper = styled<{}, 'div'>('div')`
display: flex;
`
