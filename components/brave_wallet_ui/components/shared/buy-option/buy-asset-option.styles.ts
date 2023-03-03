// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { WalletButton } from '../style'

export const BuyAssetOptionWrapper = styled(WalletButton)<{ isSelected?: boolean }>`
  width: 100%;
  height: 76px;
  box-sizing: border-box;
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  cursor: pointer;
  border-style: solid;
  border-radius: 4px;
  background-color: ${(p) => p.theme.color.background02};
  border-color: ${(p) => p.isSelected ? p.theme.color.focusBorder : p.theme.color.divider01};
  border-width: ${(p) => p.isSelected ? '3px' : '1px'};
  margin-top: 6px;
  margin-bottom: 18px;
  padding: ${(p) => p.isSelected ? '11px' : '13px'};
`

export const NameAndIcon = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
`

export const AssetName = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text01};
`

export const NameColumn = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: center;
  flex-direction: column;
`

export const NetworkDescriptionText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
`

export const PriceContainer = styled.div`
  align-self: center;
  justify-self: flex-end;
`

export const PriceText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text01};
  text-align: right;
  vertical-align: middle;
`
