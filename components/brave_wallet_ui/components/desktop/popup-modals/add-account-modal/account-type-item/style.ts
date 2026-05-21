// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { Row, Text } from '../../../../shared/style'
import { layoutPanelWidth } from '../../../wallet-page-wrapper/wallet-page-wrapper.style'

export const StyledWrapper = styled(Row)`
  --leo-icon-color: ${leo.color.icon.default};
  cursor: pointer;
  padding: 24px;
  &:hover {
    background-color: ${leo.color.container.highlight};
  }
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 18px;
  }
`

export const Title = styled(Text)`
  font: ${leo.font.default.semibold};
  letter-spacing: ${leo.typography.letterSpacing.default};
`

export const Description = styled(Text)`
  font: ${leo.font.default.regular};
  letter-spacing: ${leo.typography.letterSpacing.default};
`

export const NetworkIcon = styled.img`
  width: 45px;
  height: 45px;
`
