// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled, { DefaultTheme, ThemedStyledProps } from 'styled-components'
import LeoProgressBar from '@brave/leo/react/progressBar'
import * as leo from '@brave/leo/tokens/css'

export const BarAndMessageContainer = styled.div`
  width: 100%;
  box-sizing: border-box;
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  padding: 0px;
  gap: 16px;
`

export const ProgressBar = styled(LeoProgressBar)<{ criteria: boolean[] }>`
  width: 100%;
  --leo-progressbar-height: 4px;
  --leo-progressbar-radius: ${leo.spacing.m};
  --leo-progressbar-color: ${(p) => getCriteriaPercentColor(p)};
  --leo-progressbar-background-color: ${leo.color.container.highlight};
`

// floating tooltip positioner
export const BarProgressTooltipContainer = styled.div<{
  criteria: boolean[]
}>`
  width: 100%;
  z-index: 200;
  transform: translateX(50%);
`

export const BarMessage = styled.p<{ criteria: boolean[] }>`
  color: ${(p) => getCriteriaPercentColor(p)};
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 500;
  font-size: 12px;
  line-height: 20px;
  display: flex;
  align-items: center;
  text-align: right;
  letter-spacing: 0.01em;
`
const getCriteriaPercentColor = (
  p: ThemedStyledProps<
    {
      criteria: boolean[]
    },
    DefaultTheme
  >
) => {
  const percentComplete =
    (p.criteria.filter((c) => !!c).length / p.criteria.length) * 100
  return percentComplete === 100
    ? leo.color.systemfeedback.successIcon
    : percentComplete < 50
    ? leo.color.systemfeedback.errorIcon
    : leo.color.systemfeedback.warningIcon
}
