// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { StyledButton } from '../../../page/screens/send/shared.styles'
import { layoutPanelWidth } from '../../desktop/wallet-page-wrapper/wallet-page-wrapper.style'

export const ButtonsContainer = styled.div<{
  width?: number
}>`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  box-sizing: border-box;
  overflow: hidden;
  padding: 4px;
  width: ${(p) => (p.width !== undefined ? `${p.width}px` : '100%')};
  background-color: ${leo.color.container.highlight};
  border-radius: 16px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    border-radius: 12px;
  }
`

export const Button = styled(StyledButton)<{
  isSelected: boolean
}>`
  --selected-background-color: ${leo.color.container.background};
  background-color: ${(p) =>
    p.isSelected ? 'var(--selected-background-color)' : 'none'};
  border-radius: 12px;
  font-weight: 600;
  font-size: 12px;
  line-height: 20px;
  letter-spacing: 0.4px;
  width: 100%;
  padding: 10px;
  color: ${(p) =>
    p.isSelected ? leo.color.text.primary : leo.color.text.secondary};
  box-shadow: ${(p) => (p.isSelected ? leo.effect.elevation['01'] : 'none')};
  @media screen and (max-width: ${layoutPanelWidth}px) {
    border-radius: 8px;
    padding: 5px;
  }
`
