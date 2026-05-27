// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Icon from '@brave/leo/react/icon'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { Column, Text } from '../../shared/style'
import { ConfirmationButtonLink } from '../shared-panel-styles'

export const StyledWrapper = styled(Column)`
  background-color: ${leo.color.page.background};
`

export const Card = styled(Column)`
  background-color: ${leo.color.container.background};
  border-radius: ${leo.radius.xl};
  box-shadow: ${leo.effect.elevation['01']};
  overflow: hidden;
`

export const Title = styled(Text)`
  font: ${leo.font.heading.h4};
`

export const PriorityLabel = styled(Text)`
  font: ${leo.font.default.semibold};
`

export const FeeButton = styled(ConfirmationButtonLink)`
  font: ${leo.font.default.semibold};
`

export const InfoText = styled(Text)`
  font: ${leo.font.small.regular};
`

export const InfoIcon = styled(Icon)`
  --leo-icon-size: 16px;
`
