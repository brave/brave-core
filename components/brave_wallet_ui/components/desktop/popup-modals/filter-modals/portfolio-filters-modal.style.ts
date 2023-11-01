// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// Constants
import { layoutPanelWidth } from '../../wallet-page-wrapper/wallet-page-wrapper.style'

// Shared Styles
import { Column, Row } from '../../../shared/style'

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
