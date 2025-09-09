// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { Text } from '../../shared/style'

export const HeaderText = styled(Text)`
  font: ${leo.font.heading.h4};
  letter-spacing: ${leo.typography.letterSpacing.headings};
`
