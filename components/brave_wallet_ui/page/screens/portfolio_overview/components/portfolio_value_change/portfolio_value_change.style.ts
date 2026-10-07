// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { Text } from '$wallet/components/shared/style'

export const FiatChange = styled(Text)`
  margin-right: 8px;
`

export const PercentBubble = styled.div<{ isDown?: boolean }>`
  font: ${leo.font.xSmall.regular};
  display: flex;
  padding: 4px 8px;
  border-radius: 4px;
  background-color: ${(p) =>
    p.isDown ? leo.color.red[20] : leo.color.green[20]};
  letter-spacing: 0.02em;
  color: ${(p) => (p.isDown ? leo.color.red[50] : leo.color.green[50])};
`
