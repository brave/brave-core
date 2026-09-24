// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import { Column, WalletButton, Text } from '../../../shared/style'

export const StyledWrapper = styled(Column)`
  overflow: hidden;
`

export const FeeOptionButton = styled(WalletButton)<{
  isSelected: boolean
}>`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  background: none;
  cursor: pointer;
  outline: none;
  margin: 0px;
  background-color: ${({ isSelected }) =>
    isSelected
      ? leo.color.container.background
      : leo.color.container.highlight};
  border-radius: ${leo.radius.m};
  border: 2px solid
    ${({ isSelected }) =>
      isSelected ? leo.color.button.background : 'transparent'};
  padding: 12px;
  width: 100%;
`

export const NameText = styled(Text)`
  font: ${leo.font.small.semibold};
  letter-spacing: ${leo.typography.letterSpacing.small};
`

export const TimeText = styled(Text)`
  font: ${leo.font.heading.h4};
  letter-spacing: ${leo.typography.letterSpacing.large};
`

export const FeeAmountText = styled(Text)`
  font: ${leo.font.xSmall.semibold};
  letter-spacing: ${leo.typography.letterSpacing.small};
  word-break: break-all;
`

export const FiatAmountText = styled(Text)`
  font: ${leo.font.small.regular};
  letter-spacing: ${leo.typography.letterSpacing.small};
`

export const RadioIcon = styled(Icon)<{
  isSelected: boolean
}>`
  --leo-icon-size: 18px;
  --leo-icon-color: ${({ isSelected }) =>
    isSelected ? leo.color.icon.interactive : leo.color.icon.disabled};
`
