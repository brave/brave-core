/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*/

import { Type } from './index'
import styled from 'styled-components'

interface StyleProps {
  modal?: boolean
  type?: Type
}

export const StyledWrapper = styled('div')<{}>`
  width: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  background: ${p => p.theme.palette.blue000};
  padding: 16px;
  box-shadow: 0 2px 4px rgba(0,0,0,0.2);
  border-radius: 4px;
`

export const StyledAlertIcon = styled('div')<{}>`
  width: 24px;
  height: 24px;
  margin: 8px;
  color: ${p => p.theme.palette.blue500};
`

export const StyledInfo = styled('div')<StyleProps>`
  font-size: 16px;
  font-family: ${p => p.theme.fontFamily.body};
  color: ${p => p.theme.color.text};
  display: flex;
  align-items: center;
`

export const StyledMessage = styled('span')<StyleProps>`
  display: flex;
  align-items: center;
`

export const StyledMonthlyTips = styled('span')<StyleProps>`
  padding: 4px;
  display: flex;
  align-items: center;
`

export const StyledReviewWrapper = styled('div')<{}>`
  padding: 4px;
  display: flex;
  align-items: center;
`

export const StyledReviewList = styled('span')<{}>`
  color: #15A4FA;
  cursor: pointer;
  font-size: 14px;
  font-weight: 500;
  letter-spacing: 0;
  line-height: 18px;
`

export const StyledModalContent = styled('div')<{}>`
  display: block;
`

export const StyledTipsIcon = styled('div')<{}>`
  width: 20%;
  vertical-align: top;
  margin-top: -33px;
  display: inline-block;
`

export const StyledModalInfo = styled('div')<{}>`
  width: 80%;
  padding-left: 20px;
  display: inline-block;
`

export const StyledListMessage = styled('div')<{}>`
  display: block;
  font-size: 16px;
  font-weight: 600;
  margin-top: 30px;
`

export const StyledList = styled('ul')<{}>`
  display: block;
  font-size: 14px;
  font-weight: 300;
  padding-left: 20px;
`

export const StyledListItem = styled('li')<{}>`
  display: block;
  display: list-item;
  line-height: 28px;
  list-style-type: disc;
`

export const StyledButton = styled('div')<{}>`
  width: 235px;
  margin: 40px auto 0 auto;
`

export const StyledButtonContainer = styled('div')<{}>`
  width: 100%;
`
