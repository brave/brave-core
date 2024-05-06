// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'
import { WalletButton } from '../../../../components/shared/style'
import {
  layoutSmallWidth //
} from '../../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper.style'

export const Button = styled(WalletButton)`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  border: none;
  outline: none;
  background-color: ${leo.color.container.background};
  box-shadow: 0px 1px 4px 0px rgba(0, 0, 0, 0.07);
  position: absolute;
  z-index: 1;
  cursor: pointer;
`

export const ComposerButtonMenu = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  background-color: ${leo.color.container.background};
  border-radius: 12px;
  overflow: hidden;
  box-shadow: 0px 1px 4px 0px rgba(0, 0, 0, 0.07);
  position: absolute;
  z-index: 10;
  top: -23.5px;
`

export const ComposerButton = styled(Button)`
  justify-content: flex-start;
  font-family: Poppins;
  font-size: 13px;
  font-style: normal;
  font-weight: 600;
  line-height: 21px;
  color: ${leo.color.text.interactive};
  letter-spacing: 0.39px;
  padding: 12px 16px;
  box-shadow: none;
  position: unset;
  z-index: unset;
  width: 100%;
`

export const FlipButton = styled(Button)`
  padding: 10px;
  border-radius: 100%;
  left: 48px;
  :disabled {
    cursor: not-allowed;
  }
  @media screen and (max-width: ${layoutSmallWidth}px) {
    left: 16px;
  }
`

export const SettingsButton = styled(Button)`
  padding: 10px;
  border-radius: 100%;
  right: 48px;
  @media screen and (max-width: ${layoutSmallWidth}px) {
    right: 16px;
  }
`

export const CaratIcon = styled(Icon).attrs({
  name: 'carat-down'
})<{ isOpen: boolean }>`
  --leo-icon-size: 20px;
  color: ${leo.color.icon.interactive};
  margin-left: 8px;
  transition-duration: 0.3s;
  transform: ${(p) => (p.isOpen ? 'rotate(180deg)' : 'unset')};
`

export const SettingsIcon = styled(Icon).attrs({
  name: 'settings'
})`
  --leo-icon-size: 20px;
  color: ${leo.color.icon.default};
`

export const FlipIcon = styled(Icon).attrs({
  name: 'swap-vertical'
})`
  --leo-icon-size: 20px;
  color: ${leo.color.icon.default};
`
