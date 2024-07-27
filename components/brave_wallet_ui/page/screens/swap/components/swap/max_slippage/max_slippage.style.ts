// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Input from '@brave/leo/react/input'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import {
  WalletButton //
} from '../../../../../../components/shared/style'

export const OptionButton = styled(WalletButton)<{
  isSelected: boolean
}>`
  cursor: pointer;
  display: flex;
  flex-direction: row;
  justify-content: space-between;
  background: none;
  background-color: ${(p) =>
    p.isSelected ? 'none' : leo.color.container.highlight};
  outline: ${(p) =>
    p.isSelected ? `solid 2px ${leo.color.button.background}` : 'none'};
  border: none;
  border-radius: 8px;
  width: 100%;
  height: 100%;
  min-height: 95px;
  padding: 12px;
`

export const CustomInput = styled(Input)`
  max-width: 100px;
  text-align: right;
  --leo-control-icon-size: 14px;
`

export const RadioIcon = styled(Icon)<{
  isSelected: boolean
}>`
  --leo-icon-size: 20px;
  color: ${(p) =>
    p.isSelected ? leo.color.icon.interactive : leo.color.icon.default};
`
