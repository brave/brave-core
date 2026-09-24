// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Button from '@brave/leo/react/button'
import Tooltip from '@brave/leo/react/tooltip'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import {
  AssetIconFactory,
  AssetIconProps,
  Text,
  Column,
  WalletButton,
} from '../../shared/style'
import { ConfirmationButtonLink } from '../shared-panel-styles'

export const IconsWrapper = styled(Column)`
  position: relative;
`

export const AssetIcon = AssetIconFactory<AssetIconProps>({
  width: '40px',
  height: 'auto',
})

export const NetworkIconWrapper = styled.div`
  display: flex;
  position: absolute;
  bottom: -2px;
  right: -2px;
  background-color: ${leo.color.container.background};
  border-radius: 100%;
  padding: 2px;
`

export const TokenAmountText = styled(Text)`
  font: ${leo.font.heading.h3};
  letter-spacing: ${leo.typography.letterSpacing.headings};
`

export const AccountButton = styled(WalletButton)`
  cursor: pointer;
  padding: 0px;
  margin: 0px;
  outline: none;
  border: none;
  background: transparent;
`

export const ArrowIconContainer = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  min-width: 40px;
  min-height: 40px;
  border-radius: 100%;
  background-color: ${leo.color.page.background};
  --leo-icon-size: 16px;
  --leo-icon-color: ${leo.color.icon.default};
`

export const AddressText = styled(Text)`
  font: ${leo.font.large.semibold};
  letter-spacing: ${leo.typography.letterSpacing.large};
`

export const WarningTooltip = styled(Tooltip)`
  --leo-icon-size: 16px;
  --leo-icon-color: ${leo.color.icon.default};
`

export const WarningTooltipContent = styled(Column)`
  white-space: pre-line;
`

export const LearnMoreButton = styled(ConfirmationButtonLink)`
  display: inline-flex;
`

export const BlockExplorerButton = styled(Button)`
  --leo-icon-size: 20px;
  color: ${leo.color.icon.default};
`
