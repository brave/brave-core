// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { Column, Text } from '../../shared/style'

export const StyledWrapper = styled(Column)`
  background-color: ${leo.color.page.background};
`

export const IconWrapper = styled(Column)`
  --leo-icon-size: 88px;
  --leo-icon-color: ${leo.color.icon.default};
  border-radius: 100%;
  background-color: ${leo.color.container.highlight};
`

export const Title = styled(Text)`
  font: ${leo.font.heading.h4};
  letter-spacing: ${leo.typography.letterSpacing.large};
`

export const EmptySpace = styled.div`
  display: flex;
  height: 44px;
  width: 100%;
`
