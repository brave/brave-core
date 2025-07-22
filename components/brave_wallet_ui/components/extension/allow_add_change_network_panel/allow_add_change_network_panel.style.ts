// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import { WalletButton, Column, Text, Row } from '../../shared/style'

export const StyledWrapper = styled(Column)`
  background-color: ${leo.color.page.background};
`

export const HeaderText = styled(Text)`
  font: ${leo.font.heading.h4};
  letter-spacing: ${leo.typography.letterSpacing.large};
`

export const OriginText = styled(Text)`
  font: ${leo.font.small.regular};
  letter-spacing: ${leo.typography.letterSpacing.small};
`

export const Title = styled(Text)`
  font: ${leo.font.heading.h3};
  letter-spacing: ${leo.typography.letterSpacing.headings};
`

export const Description = styled(Text)`
  font: ${leo.font.default.regular};
  letter-spacing: ${leo.typography.letterSpacing.default};
`

export const NetworkInfoBox = styled(Column)`
  background-color: ${leo.color.container.background};
  border-radius: ${leo.radius.xl};
`

export const NetworkInfoLabel = styled(Text)`
  font: ${leo.font.small.regular};
  letter-spacing: ${leo.typography.letterSpacing.small};
`

export const NetworkInfoText = styled(Text)`
  font: ${leo.font.default.semibold};
  letter-spacing: ${leo.typography.letterSpacing.default};
`

export const FavIcon = styled.img`
  width: 64px;
  height: 64px;
  border-radius: 16px;
  background-color: ${leo.color.container.background};
`

export const DividerWrapper = styled(Row)`
  align-items: center;
  justify-content: center;
  position: relative;
`

export const ArrowIconWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  position: absolute;
  width: 24px;
  height: 24px;
  background-color: ${leo.color.container.background};
  border: 1px solid ${leo.color.divider.subtle};
  border-radius: 100%;
`

export const ArrowIcon = styled(Icon).attrs({
  name: 'carat-down',
})`
  --leo-icon-size: 16px;
  color: ${leo.color.icon.default};
`

export const DetailsButton = styled(WalletButton)`
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

export const LearnMoreButton = styled(DetailsButton)`
  font: ${leo.font.default.semibold};
  letter-spacing: ${leo.typography.letterSpacing.default};
`
