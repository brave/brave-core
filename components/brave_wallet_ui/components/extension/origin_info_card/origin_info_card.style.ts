// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { Row, Text } from '../../shared/style'

export const StyledWrapper = styled(Row)`
  background-color: ${leo.color.container.highlight};
`

export const FavIcon = styled.img`
  width: 48px;
  height: 48px;
  border-radius: 8px;
  background-color: ${leo.color.container.background};
  border: 1px solid ${leo.color.divider.subtle};
`

export const OriginName = styled(Text)`
  font: ${leo.font.default.semibold};
  letter-spacing: ${leo.typography.letterSpacing.default};
`

export const OriginUrl = styled(Text)`
  font: ${leo.font.small.regular};
  letter-spacing: ${leo.typography.letterSpacing.small};
`
