/* This Source Code Form is subject to the terms of the Mozilla Public
* License. v. 2.0. If a copy of the MPL was not distributed with this file.
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledDateTH = styled('th')<{}>`
  text-transform: uppercase;
  text-align: left;
  font-family: ${p => p.theme.fontFamily.body};
  font-size: 12px;
  font-weight: 500;
  border-bottom: 2px solid ${p => p.theme.color.separatorLine};
  border-top: 2px solid ${p => p.theme.color.separatorLine};
  color: ${p => p.theme.color.text};
  padding: 12px 0;
  min-width: 55px;
`

export const StyledAdTH = styled('th')<{}>`
  text-transform: uppercase;
  text-align: left;
  font-family: ${p => p.theme.fontFamily.body};
  font-size: 12px;
  font-weight: 500;
  border-bottom: 2px solid ${p => p.theme.color.separatorLine};
  border-top: 2px solid ${p => p.theme.color.separatorLine};
  color: ${p => p.theme.color.text};
  padding: 12px 0;
  min-width: 550px;
`

export const StyledCategoryTH = styled('th')<{}>`
  text-transform: uppercase;
  text-align: left;
  font-family: ${p => p.theme.fontFamily.body};
  font-size: 12px;
  font-weight: 500;
  border-bottom: 2px solid ${p => p.theme.color.separatorLine};
  border-top: 2px solid ${p => p.theme.color.separatorLine};
  color: ${p => p.theme.color.text};
  padding: 12px 0;
  min-width: 220px;
`

export const StyledOuterTD = styled('td')<{}>`
  width: 100%;
`

export const StyledTR = styled('tr')<{}>`
  text-align: left;
`

export const StyledAdLink = styled('a')<{}>`
  text-decoration: none;
  cursor: pointer;
  display: flex;
  color: ${p => p.theme.color.text};
`

export const StyledAdTable = styled('table')<{}>`
  min-width: 100%;
  margin: 24px 0;
  padding-top: 15px;
`
export const StyledAdContentDiv = styled('div')<{}>`
  min-width: 550px;
  border: 1px solid;
  border-collapse: separate;
  border-radius: 5px;
  border-color: ${p => p.theme.color.subtle};
  padding: 10px;
  display: inline-flex;
  align-items: center;
  height: 85px;
`

export const StyledLogo = styled('img')<{}>`
  width: 64px;
  height: 64px;
`

export const StyledLogoDiv = styled('div')<{}>`
  display: inline-block;
  width: 64px;
  height: 64px;
  padding: 1px;
  padding-right: 10px;
`

export const StyledNoLogoDiv = styled('div')<{}>`
  display: none;
`

export const StyledAdInfoDiv = styled('div')<{}>`
  padding-left: 20px;
  display: inline-block;
`
export const StyledAdInfo = styled('div')<{}>`
  display: block;
  color: ${p => p.theme.color.detailDescription};
  padding-bottom: 2px;
`

export const StyledAdBrand = styled('div')<{}>`
  font-family: ${p => p.theme.fontFamily.heading};
  font-weight: 600;
  padding-bottom: 2px;
`

export const StyledAdStatDiv = styled('div')<{}>`
  margin-left: auto;
  height: 100%;
  display: flex;
  flex-direction: column;
  padding-top: 5px;
  padding-bottom: 5px;
`

export const StyledAdStat = styled('div')<{}>`
  padding-left: 2px;
`

export const StyledAdStatActions = styled('div')<{}>`
  margin-top: auto;
`

export const StyledCategoryContentDiv = styled('div')<{}>`
  display: flex;
  flex-direction: column;
  height: 85px;
  min-width: 220px;
  padding: 15px;
`

export const StyledCategoryName = styled('div')<{}>`
  font-size: 16px;
  font-weight: 600;
  margin: auto;
`

export const StyledCategoryActions = styled('div')<{}>`
  margin: auto;
`

export const StyledNoAdHistoryDiv = styled('div')<{}>`
  text-decoration: none;
  font-size: 16px;
  font-weight: 400;
  padding-top: 24px;
`

export const StyledTD = styled('td')<{}>`
  width: 15%;
  border: 'none';
`
