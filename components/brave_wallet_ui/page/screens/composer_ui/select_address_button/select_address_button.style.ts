// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import { WalletButton, Text } from '../../../../components/shared/style'

export const Button = styled(WalletButton) <{
  isPlaceholder: boolean
}>`
  cursor: pointer;
  display: flex;
  outline: none;
  border: none;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  background-color: transparent;
  border-radius: 12px;
  padding: 10px 12px;
  white-space: nowrap;
  width: ${(p) => (p.isPlaceholder ? 'unset' : '100%')};
  &:disabled {
    opacity: 0.4;
    cursor: not-allowed;
  }
  &:hover:not([disabled]) {
    background-color: ${leo.color.container.background};
  }
`

export const ButtonIcon = styled(Icon).attrs({
  name: 'carat-right'
})`
  --leo-icon-size: 24px;
  color: ${leo.color.icon.default};
  margin-left: 8px;
`

export const ButtonText = styled(Text) <{
  isPlaceholder: boolean
}>`
  overflow: hidden;
  color: ${(p) =>
    p.isPlaceholder ? leo.color.text.tertiary : leo.color.text.primary};
  white-space: pre-wrap;
  word-break: break-all;
  font-weight: 600;
`
