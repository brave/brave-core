// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Shared Styled
import { Column, Row } from '../../../shared/style'

export const StyledWrapper = styled(Column)`
  overflow: hidden;
`

export const AccountRow = styled(Row)`
  border-radius: ${leo.radius.xl};
  background-color: ${leo.color.container.highlight};
`

export const ShieldIconWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  width: 40px;
  height: 40px;
  border-radius: 100%;
  background-color: ${leo.color.systemfeedback.successBackground};
  position: absolute;
  left: -20px;
`

export const ShieldIcon = styled(Icon).attrs({
  name: 'shield-done-filled'
})`
  --leo-icon-size: 24px;
  color: ${leo.color.systemfeedback.successIcon};
`
