// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// Shared Styles
import { StyledButton } from '../../shared.styles'

export const ButtonsContainer = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  box-sizing: border-box;
  overflow: hidden;
  padding: 4px 0px;
  width: 210px;
  height: 44px;
  background-color: ${(p) => p.theme.color.background01};
  border-radius: 100px;
  position: relative;
`

export const Button = styled(StyledButton) <{
  isSelected: boolean
  buttonAlign: 'left' | 'right'
}>`
  --selected-background-color: ${(p) => p.theme.color.interactive05};
  @media (prefers-color-scheme: dark) {
    --selected-background-color: ${(p) => p.theme.palette.blurple500};
  }
  background-color: ${(p) =>
    p.isSelected ? 'var(--selected-background-color)' : 'none'};
  border-radius: 100px;
  font-weight: 500;
  font-size: 14px;
  line-height: 20px;
  padding: 8px 16px;
  position: absolute;
  left: ${(p) => (p.buttonAlign === 'left' ? '4px' : 'unset')};
  right: ${(p) => (p.buttonAlign === 'right' ? '4px' : 'unset')};
  color: ${(p) =>
    p.isSelected ? p.theme.palette.white : p.theme.color.text02};
  z-index: ${(p) => (p.isSelected ? '10px' : '5px')};
`
