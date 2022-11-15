/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledWrapper = styled('div')<{}>`
  position: relative;
  display: flex;
  border-bottom: solid 1px #E5E5EA;
  justify-content: space-between;
  font-family: ${p => p.theme.fontFamily.body};
  align-items: center;
`

export const StyledTitle = styled('div')<{}>`
  font-size: 16px;
  color: #4b4c5c;
  padding: 16px 0;
  font-weight: 500;
`

export const StyledContentWrapper = styled('div')<{}>`
  display: flex;
  text-align: right;
`
