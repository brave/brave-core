// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// Leo styles
import * as leo from '@brave/leo/tokens/css/variables'

// Leo icons
import Icon from '@brave/leo/react/icon'

// Shared Styles
import { Row } from '../../shared.styles'

export const StyledRow = styled(Row)`
  width: 100%;
  padding: ${leo.spacing.m};
  gap: 10px;
  background-color: ${leo.color.systemfeedback.warningBackground};
  border-radius: ${leo.radius.m};
  margin-top: ${leo.spacing.xl};
  margin-bottom: ${leo.spacing.xl};
`

export const WarningIcon = styled(Icon).attrs({
  name: 'warning-triangle-filled'
})`
  --leo-icon-size: 20px;
  color: ${leo.color.systemfeedback.warningIcon};
`

export const CheckboxWrapper = styled.div`
  display: flex;
  align-items: center;
  margin-left: -8px;
`
