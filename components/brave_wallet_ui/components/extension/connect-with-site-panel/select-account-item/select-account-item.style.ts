// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as leo from '@brave/leo/tokens/css'
import Icon from '@brave/leo/react/icon'
import styled from 'styled-components'
import { WalletButton } from '../../../shared/style'

export const ConnectPanelButton = styled(WalletButton)<{
  border?: 'top' | 'bottom'
}>`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  cursor: pointer;
  outline: none;
  border: none;
  background: none;
  padding: 8px 0px;
  border-top: ${(p) =>
    p.border === 'top' ? `1px solid ${leo.color.divider.subtle}` : 'none'};
  border-bottom: ${(p) =>
    p.border === 'bottom' ? `1px solid ${leo.color.divider.subtle}` : 'none'};
`

export const NameAndAddressColumn = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  flex-direction: column;
  margin-left: 12px;
`

export const LeftSide = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  flex-direction: row;
`

export const AccountCircle = styled.div<{
  orb: string
}>`
  width: 40px;
  height: 40px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
`

export const AccountNameText = styled.span`
  font-family: 'Poppins';
  font-style: normal;
  font-size: 14px;
  line-height: 24px;
  font-weight: 600;
  color: ${leo.color.text.primary};
`

export const AccountAddressText = styled.span`
  font-family: 'Poppins';
  font-style: normal;
  font-size: 12px;
  line-height: 18px;
  font-weight: 400;
  color: ${leo.color.text.primary};
`

export const BalanceText = styled.span`
  font-family: 'Poppins';
  font-style: normal;
  font-size: 12px;
  line-height: 18px;
  font-weight: 400;
  color: ${leo.color.text.secondary};
`

export const SelectedIcon = styled(Icon)<{ isSelected: boolean }>`
  --leo-icon-size: 20px;
  color: ${(p) =>
    p.isSelected ? leo.color.primary[40] : leo.color.icon.default};
`
