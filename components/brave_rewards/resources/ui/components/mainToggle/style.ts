/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from '../../../theme'

export const MainToggleWrapper = styled<{}, 'div'>('div')`
  display: flex;
  border-radius: 4px;
  background-color: #FFF;
  justify-content: space-between;
  padding: 20px 32px;
  margin-bottom: 24px;
  width: 100%;
  box-shadow: 0 2px 4px rgba(0, 0, 0, .2);
  flex-direction: column;
`

export const ToggleHeading = styled<{}, 'div'>('div')`
  display: flex;
  align-items: center;
  width: 100%;
  justify-content: space-between;
`

export const StyledTitle = styled<{}, 'div'>('div')`
  font-size: 28px;
  font-weight: 600;
  font-family: ${p => p.theme.fontFamily.heading};
`

export const StyledLogotypeWrapper = styled('div')`
  display: flex;
  align-items: center;
`

export const StyledTOSWrapper = styled<{}, 'div'>('div')`
  display: block;
  margin-top: 20px;
  font-family: Muli, sans-serif;
`

export const StyledServiceText = styled<{}, 'span'>('span')`
  color: ${p => p.theme.palette.grey800};
  font-size: 14px;
  font-weight: normal;
  letter-spacing: 0;
  line-height: 18px;
`

export const StyledServiceLink = styled<{}, 'a'>('a')`
  cursor: pointer;
  color: ${p => p.theme.palette.blurple500};
  font-weight: 600;
`

export const StyledLogoWrapper = styled<{}, 'div'>('div')`
width: 48px;
height: 48px;
margin: 8px;
`
