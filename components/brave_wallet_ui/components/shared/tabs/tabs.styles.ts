// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { WalletButton } from '../style'
import * as leo from '@brave/leo/tokens/css/variables'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: row;
  justify-content: flex-start;
  align-items: center;
  gap: 6px;
`

export const TabWrapper = styled(WalletButton)`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  border: none;
  background: transparent;
  cursor: pointer;
`

export const Tab = styled.div<{ isActive: boolean }>`
  display: flex;
  flex-direction: row;
  align-items: center;
  gap: 8px;
  font: ${leo.font.default.semibold};
color: ${(p) =>
    p.isActive ? leo.color.text.interactive : leo.color.text.tertiary};
  margin-bottom: 12px;
`

export const Indicator = styled.div<{ isActive: boolean }>`
  width: 100%;
  height: 4px;
  background-color: ${(p) =>
    p.isActive ? leo.color.button.background : 'transparent'};
  border-radius: 2px 2px 0px 0px;
`

export const LabelSummary = styled.span<{ isActive: boolean }>`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  padding: 4px;
  min-width: 20px;
  height: 20px;
  border-radius: 6px;
  padding: 4px;
  font: ${leo.font.xSmall.regular};
display: flex;
  align-items: center;
  text-transform: uppercase;
  color: ${(p) =>
    p.isActive ? leo.color.text.interactive : leo.color.neutral['50']};
  text-align: center;
  border: 1px solid
    ${(p) => (p.isActive ? leo.color.primary['40'] : leo.color.neutral['50'])};
`
