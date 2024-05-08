// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

import LeoTooltip from '@brave/leo/react/tooltip'
import Icon from '@brave/leo/react/icon'
import * as leo from '@brave/leo/tokens/css/variables'

export const Tooltip = styled(LeoTooltip)`
  --leo-tooltip-background: ${leo.color.container.background};
  --leo-tooltip-text-color: red;
  --leo-tooltip-padding: 16px;
  border-radius: ${leo.spacing.none};
  --leo-tooltip-shadow: 0px 4px 16px -2px rgba(0, 0, 0, 0.1),
    0px 1px 0px 0px rgba(0, 0, 0, 0.05);
`

export const CriteriaCheckContainer = styled.div`
  width: 100%;
  display: flex;
  flex: 1;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  gap: 4px;
`

export const PasswordStrengthText = styled.p<{ isStrong?: boolean }>`
  font: ${leo.font.default.regular};
  margin-bottom: 6px;
  vertical-align: middle;
  color: ${leo.color.text.primary};
  padding: 0;
  margin: 0;
`

export const PasswordStrengthHeading = styled(PasswordStrengthText)`
  font: ${leo.font.default.regular};
  color: ${leo.color.text.primary};
  margin-bottom: ${leo.spacing.xs};
`

export const GreenCheckmarkIcon = styled(Icon).attrs({
  name: 'check-circle-outline'
})`
  --leo-icon-size: ${leo.spacing.xl};
  --leo-icon-color: ${leo.color.systemfeedback.successIcon};
  margin-right: ${leo.spacing.s};
`

export const CloseCircleIcon = styled(Icon).attrs({
  name: 'close-circle'
})`
  --leo-icon-size: ${leo.spacing.xl};
  --leo-icon-color: ${leo.color.systemfeedback.errorIcon};
  margin-right: ${leo.spacing.s};
`
