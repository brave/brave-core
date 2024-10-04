// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import { WalletButton } from '../../shared/style'
import {
  layoutSmallWidth //
} from '../wallet-page-wrapper/wallet-page-wrapper.style'

export const HeaderTitle = styled.span<{
  isPanel?: boolean
}>`
  font-family: Poppins;
  font-style: normal;
  font-size: ${(p) => (p.isPanel ? 16 : 28)}px;
  font-weight: ${(p) => (p.isPanel ? 600 : 500)};
  line-height: ${(p) => (p.isPanel ? 24 : 40)}px;
  color: ${leo.color.text.primary};
  word-break: break-all;
`

export const MenuWrapper = styled.div`
  position: relative;
`

export const MenuButton = styled(WalletButton)<{
  marginRight?: number
}>`
  --button-border-color: ${leo.color.divider.interactive};
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  background-color: ${leo.color.container.background};
  border-radius: 8px;
  border: 1px solid var(--button-border-color);
  height: 36px;
  width: 36px;
  margin-right: ${(p) => (p.marginRight !== undefined ? p.marginRight : 0)}px;
  @media screen and (max-width: ${layoutSmallWidth}px) {
    height: 28px;
    width: 28px;
  }
`

export const MenuButtonIcon = styled(Icon)<{
  size?: number
}>`
  --leo-icon-size: ${(p) => (p.size !== undefined ? p.size : 18)}px;
  color: ${leo.color.icon.interactive};
`

export const SendButton = styled(WalletButton)`
  cursor: pointer;
  display: flex;
  flex-direction: row;
  justify-content: center;
  align-items: center;
  padding: 12px 16px;
  background: ${leo.color.button.background};
  border-radius: 1000px;
  color: ${leo.color.white};
  border: none;
`

export const HorizontalDivider = styled.div`
  width: 1px;
  height: 48px;
  background-color: ${leo.color.divider.subtle};
`
