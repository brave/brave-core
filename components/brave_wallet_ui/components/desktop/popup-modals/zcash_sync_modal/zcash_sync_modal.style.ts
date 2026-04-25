// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Progress from '@brave/leo/react/progressRing'

// Constants
import { layoutPanelWidth } from '../../wallet-page-wrapper/wallet-page-wrapper.style'

// Shared Styles
import { Column, Row } from '../../../shared/style'

export const ProgressRing = styled(Progress)`
  --leo-progressring-size: 144px;
  margin-bottom: 24px;
`

export const ContentWrapper = styled(Column)`
  padding: 16px 32px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 16px;
  }
`

export const ButtonRow = styled(Row)`
  padding: 32px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 24px 16px;
  }
`
