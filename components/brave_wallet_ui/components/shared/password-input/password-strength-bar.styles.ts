// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled, { DefaultTheme, ThemedStyledProps } from 'styled-components'

export const BarAndMessageContainer = styled.div`
  width: 100%;
  box-sizing: border-box;
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  padding: 0px 12px;
`

// outer
export const Bar = styled.div`
  position: relative;
  flex: 1;
  box-sizing: border-box;
  height: 8px;
  border-radius: 100px;
  margin-right: 40px;
`

export const BarBackground = styled.div`
  position: absolute;
  box-sizing: border-box;
  height: 8px;
  background-color: ${(p) => p.theme.color.disabled};
  opacity: 0.4;
  border-radius: 100px;
  width: 100%;
  flex: 1;
`

// inner
export const BarProgress = styled.div<{ criteria: boolean[] }>`
  position: absolute;
  display: flex;
  flex-direction: row;
  height: 8px;
  border-radius: 100px;
  
  width: ${(p) => (p.criteria.filter(c => !!c).length / p.criteria.length) * 100}%;
  background-color: ${(p) => {
    return getCriteriaPercentColor(p)
  }};
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
const getCriteriaPercentColor = (p: ThemedStyledProps<{
  criteria: boolean[]
}, DefaultTheme>) => {
  const percentComplete = (p.criteria.filter(c => !!c).length / p.criteria.length) * 100
  return percentComplete === 100
    ? p.theme.color.successBorder
    : percentComplete < 50
      ? p.theme.color.errorIcon
      : p.theme.color.warningIcon
}
