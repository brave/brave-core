// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

const isDarkTheme = (p: any) => {
  return p.theme.name === 'Brave Dark'
}

export const StyledWrapper = styled('div')<{}>`
  width: 100%;
  color: ${p => isDarkTheme(p) ? '#fff' : '#000'};
  background: ${p => isDarkTheme(p) ? '#1d2025' : '#fff'};
`

export const StyledInner = styled('div')<{}>`
  margin: 75px auto;
  background: inherit;
  border-radius: 6px;
  overflow: hidden;
  padding: 20px 0px;
  max-width: 620px;
`

export const StyledContent = styled('div')<{}>`
  max-width: 90%;
  margin: 0 auto;
  text-align: center;
`

export const StyledHeader = styled('span')<{}>`
  font-weight: bold;
  font-size: 21px;
`

export const StyledSeparator = styled('div')<{}>`
  background: #d3d3d3;
  height: 1px;
  margin-top: 10px;
`

export const StyledDisclosure = styled('div')<{}>`
  font-size: 14px;
  margin-top: 15px;
  display: block;
  text-align: left;
`

export const StyledText = styled('span')<{}>`
  margin-top: 10px;
  display: block;
`

export const StyledRewards = styled('span')<{}>`
  color: #0076ff;
  font-size: 14px;
  cursor: pointer;
`

export const StyledButtonWrapper = styled('div')<{}>`
  width: 100%;
  margin-top: 50px;
  text-align: center;
`

export const StyledButton = styled('button')<{}>`
  color: #fff;
  background: #0076ff;
  border-radius: 20px;
  height: 40px;
  width: 200px;
  font-size: 14px;
  cursor: pointer;
  border: none;
`
