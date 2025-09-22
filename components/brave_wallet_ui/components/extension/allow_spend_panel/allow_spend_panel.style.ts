// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { Column, Row, Text } from '../../shared/style'
import { ConfirmationButtonLink } from '../shared-panel-styles'

export const StyledWrapper = styled(Column)`
  background-color: ${leo.color.page.background};
`

export const ContentWrapper = styled(Column)`
  overflow: hidden;
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
  overflow: hidden;
`

export const AmountText = styled(Text)`
  font: ${leo.font.small.semibold};
  letter-spacing: ${leo.typography.letterSpacing.small};
`

export const TokenButtonLink = styled(ConfirmationButtonLink)`
  font: ${leo.font.heading.h4};
  letter-spacing: ${leo.typography.letterSpacing.headings};
`
