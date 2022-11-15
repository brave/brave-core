/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

interface StyleProps {
  isMobile?: boolean
}

export const StyledWrapper = styled('div')<{}>`
  text-align: center;
  width: 100%;
  margin: 40px 0 0;
`

export const StyledBox = styled('div')<{}>`
  border-radius: 6px;
  background-color: #f1f1f5;
  padding: 5px 10px 20px;
`

export const StyledTitle = styled('div')<{}>`
  font-size: 14px;
  font-weight: 300;
  line-height: 1.57;
  color: #838391;
  margin: 11px 0 5px;
`

export const StyledValue = styled('div')<{}>`
  font-size: 18px;
  line-height: 1.22;
  text-align: center;
  color: #c12d7c;
`

export const StyledText = styled('div')<{}>`
  font-family: Poppins, sans-serif;
  font-size: 14px;
  line-height: 1.29;
  color: #838391;
  margin: 44px 0 0;
`

export const StyledButtonWrapper = styled('div')<StyleProps>`
  margin-top: ${p => p.isMobile ? 40 : 32}px;
`
