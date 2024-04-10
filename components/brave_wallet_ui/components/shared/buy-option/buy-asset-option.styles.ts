// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import { WalletButton } from '../style'

export const AssetButton = styled(WalletButton)<{
  isSelected?: boolean
}>`
  width: 100%;
  height: 76px;
  box-sizing: border-box;
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  cursor: pointer;
  border-style: solid;
  border-radius: 8px;
  background-color: ${(p) =>
    p.isSelected ? leo.color.container.background : 'transparent'};
  border-color: ${(p) =>
    p.isSelected ? leo.color.button.background : leo.color.divider.subtle};
  border-width: 1px;
  padding: 8px 12px;
`

export const NameAndIcon = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
`

export const AssetName = styled.span`
  font-family: Poppins;
  font-size: 14px;
  font-weight: 600;
  line-height: 24px;
  color: ${leo.color.text.primary};
`

export const NameColumn = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: center;
  flex-direction: column;
`

export const NetworkDescriptionText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  color: ${leo.color.text.secondary};
`

export const PriceContainer = styled.div`
  align-self: center;
  justify-self: flex-end;
`

export const PriceText = styled.span`
  font-family: Poppins;
  font-size: 14px;
  font-weight: 600;
  line-height: 24px;
  color: ${leo.color.text.primary};
  text-align: right;
  vertical-align: middle;
`
