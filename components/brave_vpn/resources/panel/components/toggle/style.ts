// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'
import { color, font, spacing } from '@brave/leo/tokens/css/variables'

export const Content = styled.div`
  display: flex;
  padding: ${spacing['2Xl']} ${spacing.m};
  align-items: center;
  gap: ${spacing.m};
  align-self: stretch;
`

// Status indicator
export const StatusBox = styled.div`
  display: flex;
  align-items: center;
  gap: 8px;
  flex: 1 0 0;
`

export const StatusLabel = styled.span<{ color: string }>`
  font: ${font.heading.h4};
  color: ${(p) => p.color};
`

export const ActiveIndicator = styled(Icon)`
  --leo-icon-size: 20px;
  --leo-icon-color: ${color.systemfeedback.successIcon};
`

export const InActiveIndicator = styled(ActiveIndicator)`
  --leo-icon-color: ${color.icon.default};
`

export const FailedIndicator = styled(ActiveIndicator)`
  --leo-icon-color: ${color.systemfeedback.errorText};
`

export const LoadingIcon = styled(ProgressRing)`
  --leo-progressring-size: 20px;
  --leo-progressring-color: ${color.icon.interactive};
`
