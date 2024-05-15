// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import {
  AssetIconProps,
  AssetIconFactory,
  Row,
  WalletButton,
  Text,
  Column
} from '../../../../components/shared/style'

export const AssetIcon = AssetIconFactory<AssetIconProps>({
  width: '40px',
  height: 'auto'
})

export const NetworkIconWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  position: absolute;
  bottom: -3px;
  right: -3px;
  background-color: ${leo.color.container.background};
  border-radius: 100%;
  padding: 2px;
  z-index: 3;
`

export const ButtonWrapper = styled(Row)`
  background-color: transparent;
  white-space: nowrap;
  &:hover {
    background-color: ${leo.color.page.background};
  }
`

export const Button = styled(WalletButton)`
  cursor: pointer;
  display: flex;
  flex-direction: row;
  outline: none;
  border: none;
  background-color: transparent;
  justify-content: space-between;
  align-items: center;
  padding: 10px 16px;
  white-space: nowrap;
  width: 100%;
  &:disabled {
    opacity: 0.5;
    cursor: not-allowed;
  }
`

export const IconsWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  position: relative;
  margin-right: 16px;
`

export const NameAndBalanceColumn = styled(Column)`
  overflow: hidden;
`

export const TokenNameRow = styled(Row)`
  overflow: hidden;
`

export const LeftSide = styled(TokenNameRow)`
  max-width: 100%;
`

export const TokenNameText = styled(Text)`
  display: block;
  overflow: hidden;
  text-overflow: ellipsis;
  line-height: 22px;
  white-space: nowrap;
`

export const FiatBalanceText = styled(Text)`
  line-height: 22px;
`

export const TokenBalanceText = styled(Text)`
  line-height: 20px;
`

export const PercentChangeText = styled(TokenBalanceText)<{
  isDown: boolean
}>`
  color: ${(p) =>
    p.isDown
      ? leo.color.systemfeedback.errorText
      : leo.color.systemfeedback.successText};
`

export const PercentChangeIcon = styled(Icon)<{
  isDown: boolean
}>`
  --leo-icon-size: 14px;
  margin-right: 2px;
  color: ${(p) =>
    p.isDown
      ? leo.color.systemfeedback.errorIcon
      : leo.color.systemfeedback.successIcon};
`

export const DisabledLabel = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  background-color: ${leo.color.green[20]};
  color: ${leo.color.green[50]};
  font-size: 10px;
  font-weight: 700;
  text-transform: uppercase;
  font-family: 'Poppins';
  padding: 4px 6px;
  border-radius: 4px;
`

export const AccountsIcon = styled(Icon).attrs({
  name: 'user-accounts'
})`
  --leo-icon-size: 16px;
  margin-right: 8px;
  color: ${leo.color.icon.default};
`

export const InfoButton = styled(WalletButton)`
  cursor: pointer;
  background-color: none;
  background: none;
  outline: none;
  border: none;
  padding: 0px 16px 0px 0px;
`

export const InfoIcon = styled(Icon).attrs({
  name: 'info-outline'
})`
  --leo-icon-size: 15px;
  color: ${leo.color.icon.default};
`
