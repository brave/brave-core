// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import { AssetIconProps, AssetIconFactory } from '../style'

export const StackContainer = styled.div<{
  width: string
}>`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  border-radius: 4px;
  margin-right: 26px;
  width: ${(p) => p.width};
  height: 20px;
  position: relative;
  box-sizing: border-box;
`

export const IconWrapper = styled.div<{
  leftPosition: number
}>`
  position: absolute;
  left: ${(p) => p.leftPosition}px;
`

export const AssetIcon = AssetIconFactory<AssetIconProps>({
  width: '16px',
  height: 'auto'
})

export const AdditionalCountBubble = styled.div`
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  padding: 0px 4px;
  height: 16px;
  background-color: ${leo.color.container.highlight};
  color: ${leo.color.text.secondary};
  border-radius: 100px;
  font-size: 11px;
  font-family: Poppins;
  font-style: normal;
  font-weight: 400;
  line-height: 16px;
`
