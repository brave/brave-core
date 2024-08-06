// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'
import { WalletButton, Column, Row } from '../../../../../shared/style'
import { layoutSmallWidth } from '../../../../wallet-page-wrapper/wallet-page-wrapper.style'

export const ButtonsRow = styled(Row)`
  display: none;
  @media screen and (max-width: ${layoutSmallWidth}px) {
    display: flex;
  }
`

export const ButtonWrapper = styled(Column)`
  margin-right: 28px;
  &:last-child {
    margin-right: 0px;
  }
`

export const Button = styled(WalletButton)<{
  minWidth?: number
}>`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  width: 48px;
  height: 48px;
  border-radius: 16px;
  outline: none;
  border: none;
  background: none;
  margin-bottom: 4px;
  background-color: transparent;
  background-color: ${leo.color.button.background};
`

export const ButtonIcon = styled(Icon)`
  --leo-icon-size: 24px;
  color: ${leo.color.white};
  @media (prefers-color-scheme: dark) {
    color: ${leo.color.schemes.onPrimary};
  }
`

export const ButtonText = styled.span`
  font-family: Poppins;
  font-style: normal;
  font-size: 12px;
  font-weight: 600;
  line-height: 20px;
  color: ${leo.color.text.primary};
`

export const MoreMenuWrapper = styled(ButtonWrapper)`
  position: relative;
`
