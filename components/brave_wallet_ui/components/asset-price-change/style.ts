// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import { layoutPanelWidth } from '../desktop/wallet-page-wrapper/wallet-page-wrapper.style'
import { Text } from '../shared/style'

export const StyledWrapper = styled.span`
  display: flex;
  align-items: center;
`

export const PriceChange = styled(Text)`
  display: flex;
  align-items: center;
  font: ${leo.font.default.semibold};

  @media screen and (max-width: ${layoutPanelWidth}px) {
    font: ${leo.font.small.regular};
  }
`

export const Arrow = styled(Icon)<{
  isDown: boolean
}>`
  --leo-icon-size: 24px;
  color: ${(p) =>
    p.isDown
      ? leo.color.systemfeedback.errorText
      : leo.color.systemfeedback.successText};
`
