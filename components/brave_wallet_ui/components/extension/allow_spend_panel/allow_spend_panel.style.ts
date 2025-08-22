// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { Column, Row, Text, WalletButton } from '../../shared/style'

export const StyledWrapper = styled(Column)`
  background-color: ${leo.color.page.background};
`

export const HeaderText = styled(Text)`
  font: ${leo.font.heading.h4};
  letter-spacing: ${leo.typography.letterSpacing.headings};
`

export const Title = styled(Row)`
  font: ${leo.font.heading.h4};
  letter-spacing: ${leo.typography.letterSpacing.headings};
  color: ${leo.color.text.primary};
`

export const Description = styled(Text)`
  font: ${leo.font.small.regular};
  letter-spacing: ${leo.typography.letterSpacing.small};
`

export const Card = styled(Column)`
  background-color: ${leo.color.container.background};
  border-radius: ${leo.radius.xl};
  box-shadow: ${leo.effect.elevation['01']};
  overflow: hidden;
`

export const InfoBox = styled(Column)`
  background-color: ${leo.color.container.highlight};
  border-radius: ${leo.radius.xl};
`

export const InfoLabel = styled(Text)`
  font: ${leo.font.small.semibold};
  letter-spacing: ${leo.typography.letterSpacing.small};
`

export const InfoText = styled(Text)`
  font: ${leo.font.small.regular};
  letter-spacing: ${leo.typography.letterSpacing.small};
`

export const AmountText = styled(Text)`
  font: ${leo.font.small.semibold};
  letter-spacing: ${leo.typography.letterSpacing.small};
`

export const ButtonLink = styled(WalletButton)`
  --leo-icon-size: 16px;
  cursor: pointer;
  display: flex;
  flex-direction: row;
  gap: 2px;
  padding: 0px;
  margin: 0px;
  border: none;
  background: none;
  font: ${leo.font.small.semibold};
  letter-spacing: ${leo.typography.letterSpacing.small};
  color: ${leo.color.text.interactive};
`

export const TokenButtonLink = styled(ButtonLink)`
  font: ${leo.font.heading.h4};
  letter-spacing: ${leo.typography.letterSpacing.headings};
`
