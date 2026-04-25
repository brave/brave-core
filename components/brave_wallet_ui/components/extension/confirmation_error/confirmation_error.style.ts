// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { Row, Text } from '../../shared/style'

export const Wrapper = styled(Row)`
  --leo-icon-size: 16px;
  --leo-icon-color: ${leo.color.systemfeedback.errorIcon};
  background-color: ${leo.color.systemfeedback.errorBackground};
`

export const ErrorText = styled(Text)`
  font: ${leo.font.small.regular};
  letter-spacing: ${leo.typography.letterSpacing.small};
`
