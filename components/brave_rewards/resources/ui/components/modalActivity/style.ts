/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

interface TabProps {
  isFirst: boolean
  selected: boolean
}

export const StyledWrapper = styled('div')<{}>`
  font-family: ${p => p.theme.fontFamily.body};
`

export const StyledTitle = styled('div')<{}>`
  font-weight: 600;
  color: #1B1D2F;
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 16px;
  line-height: 2;
`

export const StyledSubTitle = styled('span')<{}>`
  color: #838391;
  font-weight: normal;
`

export const StyledHeader = styled('div')<{}>`
  display: flex;
  width: 100%;
  justify-content: space-between;
`

export const StyledLeft = styled('div')<{}>`
  flex-basis: 40%;
`

export const StyledRight = styled('div')<{}>`
  flex-basis: 378px;
  flex-grow: 0;
  flex-shrink: 1;
  margin-bottom: 45px;
`

export const StyledSelectOption = styled('div')<{}>`
  font-size: 22px;
  font-weight: 300;
  color: #4C54D2;
`

export const StyledIconWrap = styled('div')<{}>`
  margin-bottom: 103px;
  display: flex;
`

export const StyledIcon = styled('button')<{}>`
  display: flex;
  margin-right: 35px;
  background: none;
  border: none;
  cursor: pointer;
  align-items: center;
`

export const StyledIconText = styled('div')<{}>`
  font-size: 14px;
  line-height: 1.43;
  color: #838391;
  margin-left: 13px;
`

export const StyledTables = styled('div')<{}>`
  background-color: #f9f9fd;
  margin: 0 -48px;
  padding: 5px 50px;
  border-top: 1px solid #ebecf0;
`

export const StyledNote = styled('div')<{}>`
  max-width: 508px;
  margin-top: 46px;
  font-family: Muli, sans-serif;
  font-size: 12px;
  font-weight: 300;
  line-height: 1.5;
  color: #686978;
`

export const StyledActionIcon = styled('span')<{}>`
  color: #A1A8F2;
  width: 27px;
`

export const Tabs = styled('div')<{}>`
  width: 100%;
  justify-content: space-between;
  display: flex;
  margin-bottom: -10px;
`

export const Tab = styled('button')<TabProps>`
  flex: 1 1 0px
  font-size: 16px;
  font-weight: ${p => p.selected ? 700 : 400};
  letter-spacing: -0.29px;
  line-height: 44px;
  display: inline-block;
  cursor: pointer;
  position: relative;
  margin: 0px 2px;
  border:none;
  border-left: ${p => p.isFirst ? 'none' : '1px solid rgb(223, 223, 232)'};
  background: none;
`

export const TabContent = styled('div')<{}>`
`
