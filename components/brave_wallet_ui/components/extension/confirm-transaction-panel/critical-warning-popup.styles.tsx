// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'

import { Column } from '../../shared/style'
import Icon from '@brave/leo/react/icon'

export const LargeWarningCircleIcon = styled(Icon).attrs({
  name: 'warning-circle-filled'
})`
  --leo-icon-size: 40px;
  color: ${leo.color.systemfeedback.errorIcon};
`

export const FullWidthChildrenColumn = styled(Column)`
  align-self: flex-end;
  justify-self: flex-end;
  & * {
    width: 100%;
  }
`

export const WarningInfoCircleIcon = styled(Icon).attrs({
  name: 'warning-circle-outline'
})`
  --leo-icon-size: 16px;
  width: 16px;
  height: 16px;
  color: ${leo.color.systemfeedback.warningIcon};
`

export const WarningButtonText = styled.span`
  color: ${leo.color.systemfeedback.errorIcon};
`

export const CriticalWarningTitle = styled.span`
  color: ${leo.color.text.primary};
  text-align: center;
  font: ${leo.font.default.semibold};
`

export const CriticalWarningDescription = styled.span`
  color: ${leo.color.text.primary};
  text-align: center;
  font: ${leo.font.small.regular};
`
