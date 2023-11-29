// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import Icon from '@brave/leo/react/icon'

// shared styles
import { Row } from '../../../shared/style'

interface WarningBannerProps {
  isCritical?: boolean
}

export const AlertIcon = styled(Icon).attrs<WarningBannerProps>((props) => ({
  name: props.isCritical ? 'warning-circle-filled' : 'warning-triangle-filled'
}))<WarningBannerProps>`
  --leo-icon-size: 20px;
  padding: 0px 16px;
  color: ${(p) =>
    p.isCritical
      ? leo.color.systemfeedback.errorIcon
      : leo.color.systemfeedback.warningIcon};
`

export const WarningAlertRow = styled(Row)<WarningBannerProps>`
  min-height: 44px;
  color: ${leo.color.text.primary};
  background-color: ${(p) =>
    p.isCritical
      ? leo.color.systemfeedback.errorBackground
      : leo.color.systemfeedback.warningBackground};
`
