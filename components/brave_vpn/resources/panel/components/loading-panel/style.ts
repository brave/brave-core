// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import ProgressRing from '@brave/leo/react/progressRing'
import { color, font, spacing } from '@brave/leo/tokens/css/variables'

export const LoadingIcon = styled(ProgressRing)`
  --leo-progressring-size: 40px;
  --leo-progressring-color: ${color.icon.interactive};
`

export const Status = styled.div`
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  gap: ${spacing.m};
  padding: 35px 0;
`

export const PanelDesc = styled.div`
  color: ${color.text.secondary};
  font: ${font.default.regular};
  margin: 0;
  text-align: center;
`
