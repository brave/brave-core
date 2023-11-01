// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import Icon from '@brave/leo/react/icon'
import { WalletButton } from '../../shared/style'
import {
  layoutPanelWidth,
  layoutSmallWidth
} from '../wallet-page-wrapper/wallet-page-wrapper.style'

export const StyledWrapper = styled.div<{
  yPosition?: number
  right?: number
}>`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 8px 8px 0px 8px;
  background-color: ${leo.color.container.background};
  border-radius: 8px;
  border: 1px solid ${leo.color.divider.subtle};
  box-shadow: 0px 1px 4px rgba(0, 0, 0, 0.25);
  position: absolute;
  top: ${(p) => (p.yPosition !== undefined ? p.yPosition : 35)}px;
  right: ${(p) => (p.right !== undefined ? p.right : 0)}px;
  z-index: 20;
`

export const PopupButton = styled(WalletButton)<{
  minWidth?: number
}>`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  text-align: left;
  cursor: pointer;
  min-width: ${(p) => (p.minWidth !== undefined ? p.minWidth : 220)}px;
  border-radius: 8px;
  outline: none;
  border: none;
  background: none;
  padding: 12px 8px;
  margin: 0px 0px 8px 0px;
  background-color: transparent;
  &:hover {
    background-color: ${leo.color.divider.subtle};
  }
`

export const PopupButtonText = styled.span`
  flex: 1;
  font-family: Poppins;
  font-style: normal;
  font-size: 14px;
  font-weight: 400;
  line-height: 24px;
  color: ${leo.color.text.primary};
`

export const ButtonIcon = styled(Icon)`
  --leo-icon-size: 18px;
  color: ${leo.color.icon.default};
  margin-right: 16px;
`

export const ToggleRow = styled.label`
  display: flex;
  align-items: center;
  justify-content: space-between;
  cursor: pointer;
  width: 220px;
  padding: 12px 8px;
  margin: 0px 0px 8px 0px;
  background-color: transparent;
`

export const LineChartWrapper = styled(StyledWrapper)`
  @media screen and (max-width: ${layoutSmallWidth}px) {
    left: 0px;
    right: unset;
  }
  @media screen and (max-width: ${layoutPanelWidth}px) {
    left: unset;
    right: 0px;
  }
`
