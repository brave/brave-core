// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { Text, Column, Row, WalletButton } from '../../../../../shared/style'
import {
  layoutPanelWidth, //
} from '../../../../wallet-page-wrapper/wallet-page-wrapper.style'

export const GettingStartedWrapper = styled(Column)`
  padding: 0px 32px 32px 32px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 0px 16px 16px 16px;
  }
`

export const Title = styled(Text)`
  font: ${leo.font.heading.h4};
  letter-spacing: ${leo.typography.letterSpacing.large};
  background: ${leo.gradient.iconsActive};
  background-clip: text;
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
`

export const CardsWrapper = styled(Row)`
  @media screen and (max-width: ${layoutPanelWidth}px) {
    flex-direction: column;
  }
`

export const Card = styled(WalletButton)`
  --leo-icon-size: 32px;
  --leo-icon-color: ${leo.color.icon.interactive};
  background: none;
  border: none;
  outline: none;
  cursor: pointer;
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  gap: ${leo.spacing.l};
  padding: ${leo.spacing.l};
  border-radius: ${leo.radius.l};
  background-color: ${leo.color.container.background};
  box-shadow: ${leo.effect.elevation['01']};
`

export const CardTitle = styled(Text)`
  font: ${leo.font.large.semibold};
  letter-spacing: ${leo.typography.letterSpacing.default};
  color: ${leo.color.text.interactive};
`

export const CardDescription = styled(Text)`
  font: ${leo.font.small.regular};
  letter-spacing: ${leo.typography.letterSpacing.small};
`
