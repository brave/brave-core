// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { Row } from '$wallet/components/shared/style'

export const IconContainer = styled.div`
  --leo-icon-size: 14px;
  --leo-icon-color: ${leo.color.icon.default};
  display: flex;
  justify-content: center;
  align-items: center;
  min-width: 32px;
  min-height: 32px;
  border-radius: ${leo.radius.full};
  background-color: ${leo.color.container.highlight};
`

export const ContinueButtonWrapper = styled(Row)`
  --leo-button-color: ${leo.color.black};
  flex-grow: 0;
  @media (prefers-color-scheme: dark) {
    --leo-button-color: ${leo.color.container.highlight};
  }
  position: absolute;
  bottom: 0;
  left: 0;
  right: 0;
`
