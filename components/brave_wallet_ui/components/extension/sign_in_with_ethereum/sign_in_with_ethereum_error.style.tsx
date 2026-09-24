// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import { Text, WalletButton, Column } from '../../shared/style'

export const ErrorWrapper = styled(Column)`
  background-color: ${leo.color.systemfeedback.errorBackground};
`

export const ErrorTitle = styled(Text)`
  font: ${leo.font.heading.h4};
  letter-spacing: ${leo.typography.letterSpacing.large};
`

export const ErrorBox = styled(Column)`
  overflow-wrap: anywhere;
`

export const ErrorDescriptionText = styled(Text)`
  font: ${leo.font.default.regular};
  letter-spacing: ${leo.typography.letterSpacing.default};
  display: inline-block;
  text-align: center;
`

export const WarningIconWrapper = styled(Column)`
  border-radius: ${leo.radius.m};
  background-color: ${leo.color.container.background};
`

export const WarningIcon = styled(Icon).attrs({
  name: 'warning-circle-filled',
})`
  --leo-icon-size: 40px;
  color: ${leo.color.systemfeedback.errorIcon};
`

export const OriginErrorTitle = styled(Text)`
  font: ${leo.font.default.semibold};
  letter-spacing: ${leo.typography.letterSpacing.default};
  display: inline-block;
  text-align: center;
`

export const OriginErrorText = styled(Text)`
  font: ${leo.font.small.semibold};
  letter-spacing: ${leo.typography.letterSpacing.small};
`

export const LaunchButton = styled(WalletButton)`
  display: inline;
  margin: 0px 0px 0px 4px;
  bottom: -3px;
  position: relative;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  border: none;
  background: none;
  padding: 0px;
  background-color: transparent;
`

export const LaunchIcon = styled(Icon).attrs({
  name: 'launch',
})`
  --leo-icon-size: 18px;
  color: ${leo.color.icon.interactive};
`
