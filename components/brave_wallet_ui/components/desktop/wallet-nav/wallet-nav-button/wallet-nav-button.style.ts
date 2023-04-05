// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Icon from '@brave/leo/react/icon'
import { WalletButton, Text } from '../../../shared/style'

export const StyledButton = styled(WalletButton) <{ isSelected?: boolean }>`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  cursor: pointer;
  padding: 16px;
  outline: none;
  border: none;
  background: none;
  background-color: ${(p) => p.isSelected
    ? 'var(--nav-button-background-hover)'
    : 'none'};
  border-radius: 6px;
  margin-bottom: 8px;
  color: var(--nav-button-color);
  font-weight: 600;
  font-size: 16px;
  font-family: 'Poppins';
  &:hover {
    background-color: var(--nav-button-background-hover);
  }
  &:last-child {
    margin-bottom: 0px;
  }
  transition-duration: inherit;
`

export const ButtonIcon = styled(Icon)`
  --leo-icon-size: 24px;
  color: var(--nav-button-color);
  margin-right: var(--icon-margin-right);
  transition-duration: inherit;
`

export const ButtonText = styled(Text)`
  color: var(--nav-button-color);
  display: var(--display-text);
  transition-duration: inherit;
`
