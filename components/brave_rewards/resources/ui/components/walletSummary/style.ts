/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import palette from 'brave-ui/theme/colors'

interface StyleProps {
  compact?: boolean
}

const getGradientRule = (gradient: string) => {
  return `linear-gradient(-180deg, rgba(${gradient},1) 0%, rgba(255,255,255,1) 60%)`
}

export const StyledWrapper = styled('div')<StyleProps>`
  min-height: 315px;
  padding: ${p => p.compact ? '0px 7px 0px' : '0px'};
  background: ${p => p.compact ? getGradientRule('233, 235, 255') : 'inherit'};
`

export const StyledInner = styled('div')<StyleProps>`
  padding: 25px 14px 14px;
  font-family: Poppins, sans-serif;
`
export const StyledSummary = styled('div')<{}>`
  font-size: 14px;
  font-weight: 600;
  line-height: 1.57;
  letter-spacing: 0.4px;
  color: #a1a8f2;
  text-transform: uppercase;
`

export const StyledTitle = styled('div')<{}>`
  font-size: 22px;
  font-weight: 300;
  line-height: 0.79;
  letter-spacing: 0.4px;
  color: #4C54D2;
  margin: 4px 0 26px;
  text-transform: uppercase;
`

export const StyledActivity = styled('button')<{}>`
  font-size: 12px;
  color: #686978;
  margin-top: 26px;
  text-align: center;
  padding: 0;
  border: none;
  background: none;
  width: 100%;
  cursor: pointer;
`

export const StyledActivityIcon = styled('span')<{}>`
  vertical-align: middle;
  margin-right: 11px;
  width: 22px;
  height: 24px;
  color: #A1A8F2;
  display: inline-block;
`

export const StyledNoActivityWrapper = styled('div')<StyleProps>`
  width: 100%;
  margin-top: ${p => p.compact ? '80px' : '30px'};
  text-align: center;
`

export const StyledNoActivity = styled('span')<{}>`
  font-weight: 400;
  color: #B8B9C4;
  font-size: 18px;
`

export const StyledReservedWrapper = styled('div')<{}>`
  background: rgba(0, 0, 0, 0.04);
  color: #676283;
  font-size: 12px;
  font-family: ${p => p.theme.fontFamily.body};
  font-weight: normal;
  letter-spacing: 0;
  line-height: 16px;
  padding: 10px 12px;
  border-radius: 4px;
  margin: 20px 0 10px;
`

export const StyledReservedLink = styled('a')<StyleProps>`
  color: ${palette.blue400};
  font-weight: bold;
  text-decoration: none;
  display: inline-block;
`

export const StyledAllReserved = styled('button')<StyleProps>`
  background: none;
  border: none;
  padding: 0;
  display: block;
  margin-top: 10px;
  color: ${palette.blue400};
  cursor: pointer;
`
