// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { Column, Text } from '../../shared/style'

export const StyledWrapper = styled(Column)`
  background-color: ${leo.color.container.highlight};
  border-radius: ${leo.radius.xl};
`

export const SectionLabel = styled(Text)`
  font: ${leo.font.default.regular};
  letter-spacing: ${leo.typography.letterSpacing.default};
`

export const SectionDetails = styled(Text)`
  font: ${leo.font.default.semibold};
  letter-spacing: ${leo.typography.letterSpacing.default};
`
