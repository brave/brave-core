// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import { AssetIconProps, AssetIconFactory, WalletButton } from '../../shared/style'

export const StyledWrapper = styled.div<{isPanel?: boolean}>`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  padding: 12px ${(p) => p.isPanel ? 0 : 12}px;
  border-radius: 10px;
  &:hover {
    background-color: ${(p) => p.theme.color.background01}85;
  }
`

export const ButtonArea = styled(WalletButton) <{ disabled: boolean, rightMargin?: number }>`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  cursor: ${(p) => p.disabled ? 'default' : 'pointer'};
  outline: none;
  background: none;
  border: none;
  margin-right: ${(p) => p.rightMargin ? p.rightMargin : 0}px;
`

export const NameAndIcon = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  text-align: left;
`

export const AssetName = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text01};
`

export const BalanceColumn = styled.div`
  display: flex;
  align-items: flex-end;
  justify-content: center;
  flex-direction: column;
  text-align: right;
`

export const FiatBalanceText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text01};
`

export const AssetBalanceText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
`

// Construct styled-component using JS object instead of string, for editor
// support with custom AssetIconFactory.
//
// Ref: https://styled-components.com/docs/advanced#style-objects
const assetIconProps = {
  width: '40px',
  height: 'auto'
}
export const AssetIcon = AssetIconFactory<AssetIconProps>(assetIconProps)

export const NameColumn = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: center;
  flex-direction: column;
`

export const Spacer = styled.div`
  display: flex;
  height: 4px;
`

export const NetworkDescriptionText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
`
