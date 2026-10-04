// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import {
  Column,
  AssetIconProps,
  AssetIconFactory,
  WalletButton,
  Text,
} from '../../shared/style'

export const StyledWrapper = styled(Column)`
  background-color: ${leo.color.page.background};
`

export const HeaderText = styled(Text)`
  font: ${leo.font.heading.h4};
  letter-spacing: ${leo.typography.letterSpacing.headings};
`

export const Card = styled(Column)`
  background-color: ${leo.color.container.background};
  border-radius: ${leo.radius.xl};
  box-shadow: ${leo.effect.elevation['01']};
  overflow: hidden;
`

export const Description = styled(Text)`
  font: ${leo.font.heading.h3};
  letter-spacing: ${leo.typography.letterSpacing.headings};
`

export const TokenName = styled(Text)`
  font: ${leo.font.large.semibold};
  letter-spacing: ${leo.typography.letterSpacing.default};
`

export const TokenDescription = styled(Text)`
  font: ${leo.font.large.regular};
  letter-spacing: ${leo.typography.letterSpacing.default};
`

export const ContractAddress = styled(WalletButton)`
  font: ${leo.font.small.semibold};
  letter-spacing: ${leo.typography.letterSpacing.small};
  color: ${leo.color.text.interactive};
  background: none;
  cursor: pointer;
  outline: none;
  border: none;
  margin: 0px;
  padding: 0px;
`

export const AssetIcon = AssetIconFactory<AssetIconProps>({
  width: '80px',
  height: 'auto',
  marginBottom: '8px',
})
