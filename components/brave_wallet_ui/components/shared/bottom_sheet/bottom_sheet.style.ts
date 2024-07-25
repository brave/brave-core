// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import { Column, WalletButton } from '../style'

export const Background = styled(Column)<{ isOpen: boolean }>`
  left: 0;
  right: 0;
  top: 0;
  bottom: 0;
  position: fixed;
  z-index: ${(p) => (p.isOpen ? 31 : -1)};
  background: rgba(0, 0, 0, 0.2);
`

export const BottomCard = styled(Column)<{ isOpen: boolean }>`
  position: fixed;
  bottom: ${(p) => (p.isOpen ? 0 : -600)}px;
  transition: all 0.3s ease-in-out;
  border-radius: 16px 16px 0px 0px;
  background-color: ${leo.color.container.background};
  z-index: 32;
`

export const CloseButton = styled(WalletButton)`
  cursor: pointer;
  background-color: none;
  background: none;
  outline: none;
  border: none;
  padding: 0px;
`

export const CloseIcon = styled(Icon).attrs({
  name: 'close'
})`
  --leo-icon-size: 24px;
  color: ${leo.color.icon.default};
`
