// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

export const BubbleContainer = styled.div<{
  isV2?: boolean
}>`
  display: flex;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: space-between;
  border-radius: ${(p) => (p.isV2 ? '16px' : '12px')};
  padding: ${(p) => (p.isV2 ? '24px' : '5px 12px')};
  background-color: ${(p) =>
    p.isV2 ? leo.color.container.highlight : p.theme.color.background02};
  border: ${(p) => (p.isV2 ? 'none' : `1px solid ${p.theme.color.divider01}`)};
  margin-bottom: 12px;
  box-sizing: border-box;
`

export const SelectWrapper = styled.div`
  display: flex;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  max-height: 100%;
`

export const SelectScrollSearchContainer = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  overflow-y: auto;
  overflow-x: hidden;
  width: 100%;
  max-height: 100%;
`

export const SelectScrollContainer = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  overflow-y: auto;
  overflow-x: hidden;
  position: absolute;
  top: 50px;
  bottom: 18px;
  left: 18px;
  right: 18px;
`
