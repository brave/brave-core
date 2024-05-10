// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import {
  AssetIconFactory,
  AssetIconProps,
  Column,
  WalletButton
} from '../../../../../components/shared/style'

export const IconsWrapper = styled(Column)`
  position: relative;
`

export const AssetIcon = AssetIconFactory<AssetIconProps>({
  width: '40px',
  height: 'auto'
})

export const NetworkIconWrapper = styled(Column)`
  position: absolute;
  bottom: -3px;
  right: -3px;
  background-color: ${leo.color.container.background};
  border-radius: 100%;
  padding: 2px;
  z-index: 3;
`

export const AccountButton = styled(WalletButton)<{
  isSelected: boolean
}>`
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  padding: 16px;
  outline: none;
  background: none;
  border-radius: 12px;
  border: 1px solid
    ${(p) =>
      p.isSelected ? leo.color.button.background : leo.color.divider.subtle};
  color: none;
  transition: background-color 300ms ease-out;
  &:hover {
    background-color: ${leo.color.page.background};
  }
`
