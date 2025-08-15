// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { Column, Text } from '../../shared/style'

export const StyledWrapper = styled(Column).attrs({
  width: '100%',
  margin: '16px 0px',
  padding: '0px 16px',
  gap: '8px',
  justifyContent: 'flex-start',
  alignItems: 'flex-start',
})`
  overflow: hidden;
  overflow-y: auto;
  max-height: 300px;
`

export const DetailColumn = styled(Column).attrs({
  width: '100%',
  justifyContent: 'flex-start',
  alignItems: 'flex-start',
})``

export const LabelText = styled(Text).attrs({
  textColor: 'primary',
})`
  font: ${leo.font.default.semibold};
  letter-spacing: ${leo.typography.letterSpacing.default};
`

export const DetailText = styled(Text).attrs({
  textColor: 'tertiary',
  textAlign: 'left',
})`
  font: ${leo.font.default.regular};
  letter-spacing: ${leo.typography.letterSpacing.default};
  word-break: break-all;
`

export const NoDataText = styled(Text).attrs({
  textColor: 'tertiary',
})`
  font: ${leo.font.default.semibold};
  letter-spacing: ${leo.typography.letterSpacing.default};
`
