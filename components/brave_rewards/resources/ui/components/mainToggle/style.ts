/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const MainToggleWrapper = styled('div')<{}>`
  display: flex;
  border-radius: 4px;
  background-color: #FFF;
  justify-content: space-between;
  padding: 20px 32px;
  margin-bottom: 24px;
  width: 100%;
  flex-direction: column;
`

export const ToggleHeading = styled('div')<{}>`
  display: flex;
  align-items: center;
  width: 100%;
  justify-content: space-between;
`

export const StyledTitle = styled('div')<{}>`
  font-size: 28px;
  font-weight: 600;
  font-family: ${p => p.theme.fontFamily.heading};
`

export const StyledLogotypeWrapper = styled('div')`
  display: flex;
  align-items: center;
`

export const StyledLogoWrapper = styled('div')<{}>`
width: 48px;
height: 48px;
margin: 8px;
`
