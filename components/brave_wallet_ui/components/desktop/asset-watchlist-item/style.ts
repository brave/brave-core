// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import LeoIcon from '@brave/leo/react/icon'

// Shared Styles
import {
  AssetIconProps,
  AssetIconFactory,
  WalletButton
} from '../../shared/style'
import { layoutPanelWidth } from '../wallet-page-wrapper/wallet-page-wrapper.style'

export const assetWatchListItemHeight = 58

export const StyledWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  padding: 8px 40px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 8px 24px;
  }
`

export const NameAndIcon = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  flex-direction: row;
  max-width: 80%;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  padding: 0px;
`

export const NameAndSymbol = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: center;
  flex-direction: column;
  width: 90%;
  overflow: hidden;
  white-space: pre-line;
`

export const AssetName = styled.span`
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 600;
  color: ${(p) => p.theme.color.text01};
  text-align: left;
`

export const AssetSymbol = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text01};
  text-align: left;
`

export const Balance = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  flex-direction: row;
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text01};
  width: 48%;
`

export const BalanceColumn = styled.div`
  display: flex;
  align-items: flex-end;
  justify-content: center;
  flex-direction: column;
`

// Construct styled-component using JS object instead of string, for editor
// support with custom AssetIconFactory.
//
// Ref: https://styled-components.com/docs/advanced#style-objects
export const AssetIcon = AssetIconFactory<AssetIconProps>({
  width: '40px',
  height: 'auto'
})

export const CheckboxRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-end;
  flex-direction: row;
  width: 10%;
`

export const Button = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  width: 20px;
  height: 20px;
`

export const Icon = styled(LeoIcon)`
  --leo-icon-size: 20px;
  color: ${leo.color.icon.default};
`

export const RightSide = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
`
