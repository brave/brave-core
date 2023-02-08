// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import TrashIcon from '../../../assets/svg-icons/trash-icon.svg'
import { AssetIconProps, AssetIconFactory, WalletButton } from '../../shared/style'

export const assetWatchListItemHeight = 58

export const StyledWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  padding: 8px 0;
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

export const DeleteButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
`

export const DeleteIcon = styled.div`
  width: 18px;
  height: 18px;
  background-color: ${(p) => p.theme.color.text02};
  -webkit-mask-image: url(${TrashIcon});
  mask-image: url(${TrashIcon});
`

export const RightSide = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
`
