// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Shared Styled
import { Row, WalletButton, Column, Text } from '../../../shared/style'

export const StyledWrapper = styled(Column)`
  overflow: hidden;
`

export const AccountInfoRow = styled(Row)`
  background-color: ${leo.color.container.highlight};
  border-radius: ${leo.radius.xl};
  padding: 8px;
`

export const AddressText = styled(Text)`
  word-break: break-all;
`

export const Button = styled(WalletButton)`
  cursor: pointer;
  display: flex;
  flex-direction: row;
  justify-content: space-between;
  align-items: center;
  background: none;
  background-color: none;
  outline: none;
  border: none;
  border-radius: ${leo.radius.m};
  width: 100%;
  padding: 16px;
  --icon-display: none;
  &:hover {
    background-color: ${leo.color.container.highlight};
    --icon-display: block;
  }
`

export const LaunchIcon = styled(Icon).attrs({
  name: 'launch'
})`
  display: var(--icon-display);
  --leo-icon-size: 16px;
  color: ${leo.color.icon.interactive};
`
