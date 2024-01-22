// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import Icon from '@brave/leo/react/icon'

export const StyledWrapper = styled.div<{
  useWithFilter?: boolean
  isV2?: boolean
}>`
  --background-color: ${(p) =>
    p.isV2 ? leo.color.container.highlight : p.theme.color.background02};
  --font-size: ${(p) =>
    p.isV2 ? leo.color.container.highlight : p.theme.color.background02};
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  min-height: ${(p) => (p.isV2 ? 44 : 36)}px;
  width: 100%;
  border: ${(p) =>
    p.useWithFilter || p.isV2
      ? 'none'
      : `1px solid ${p.theme.color.interactive08}`};
  box-sizing: border-box;
  border-radius: ${(p) => (p.isV2 ? 8 : 4)}px;
  background-color: var(--background-color);
  margin-bottom: ${(p) => (p.useWithFilter || p.isV2 ? '0px' : '10px')};
  overflow: hidden;
`

export const SearchInput = styled.input<{
  useWithFilter?: boolean
  isV2?: boolean
}>`
  flex: 1;
  height: 100%;
  outline: none;
  background-image: none;
  background-color: var(--background-color);
  box-shadow: none;
  border: none;
  font-family: Poppins;
  font-style: normal;
  font-size: ${(p) => (p.useWithFilter || p.isV2 ? '14px' : '12px')};
  letter-spacing: 0.01em;
  color: ${leo.color.text.primary};
  -webkit-box-shadow: none;
  -moz-box-shadow: none;
  ::placeholder {
    font-family: Poppins;
    font-style: normal;
    font-size: ${(p) => (p.isV2 ? '14px' : '12px')};
    letter-spacing: 0.01em;
    color: ${(p) =>
      p.isV2 ? leo.color.text.tertiary : leo.color.text.secondary};
  }
  :focus {
    outline: none;
  }
  ::-webkit-inner-spin-button {
    -webkit-appearance: none;
    margin: 0;
  }
  ::-webkit-outer-spin-button {
    -webkit-appearance: none;
    margin: 0;
  }
`

export const SearchIcon = styled(Icon)<{
  isV2?: boolean
}>`
  --leo-icon-size: ${(p) => (p.isV2 ? 20 : 15)}px;
  color: ${leo.color.icon.default};
  margin-left: ${(p) => (p.isV2 ? 8 : 10)}px;
  margin-right: ${(p) => (p.isV2 ? 16 : 5)}px;
`
