// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import Icon from '@brave/leo/react/icon'
import { WalletButton } from '../../shared/style'

export const StyledWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  margin-bottom: 8px;
  padding: 8px;
  position: relative;
`

export const NameAndIcon = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
`

export const AccountAndAddress = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: center;
  flex-direction: column;
`

export const AccountNameButton = styled(WalletButton)`
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  font-weight: 600;
  color: ${leo.color.text.primary};
  margin: 0px 8px 0px 0px;
  padding: 0px;
  &:disabled {
    cursor: default;
  }
`

export const AccountAddressButton = styled(AccountNameButton)`
  font-size: 12px;
  line-height: 18px;
  font-weight: 400;
  margin-right: 6px;
`

export const AddressAndButtonRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  flex-direction: row;
`

export const BalanceColumn = styled.div`
  display: flex;
  align-items: flex-end;
  justify-content: center;
  flex-direction: column;
  margin-right: 12px;
`

export const RightSide = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
`

export const FiatBalanceText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  font-weight: 400;
  color: ${leo.color.text.secondary};
`

export const AssetBalanceText = styled.span`
  font-family: Poppins;
  font-size: 14px;
  line-height: 24px;
  font-weight: 600;
  color: ${leo.color.text.primary};
`

export const AccountMenuWrapper = styled.div`
  position: relative;
`

export const AccountMenuButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  pointer-events: auto;
  border: none;
`

export const AccountMenuIcon = styled(Icon).attrs({
  name: 'more-vertical'
})`
  --leo-icon-size: 24px;
  color: ${leo.color.icon.default};
`

export const CopyIcon = styled(Icon).attrs({
  name: 'copy'
})`
  cursor: pointer;
  --leo-icon-size: 14px;
  color: ${leo.color.icon.default};
`
