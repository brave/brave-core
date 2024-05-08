// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'
import {
  AssetIconProps,
  AssetIconFactory,
  WalletButton
} from '../../shared/style'

export const AssetButton = styled(WalletButton)<{ isERC721?: boolean }>`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  width: ${(p) => (p.isERC721 ? '100%' : 'auto')};
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  padding: 0px;
  margin: 0px;
`

// Construct styled-component using JS object instead of string, for editor
// support with custom AssetIconFactory.
//
// Ref: https://styled-components.com/docs/advanced#style-objects
export const AssetIcon = AssetIconFactory<AssetIconProps>({
  width: '24px',
  height: 'auto'
})

export const AssetTicker = styled.span<{
  role?: 'currency' | 'symbol'
}>`
  font-family: Poppins;
  font-size: 20px;
  line-height: 30px;
  letter-spacing: 0.02em;
  font-weight: 600;
  margin-right: 4px;
  color: ${(p) =>
    p.role === 'currency' ? leo.color.text.tertiary : leo.color.text.primary};
  cursor: pointer;
`

export const CaratDownIcon = styled(Icon).attrs({ name: 'carat-down' })`
  --leo-icon-size: 22px;
  color: ${(p) => leo.color.icon.default};
`

export const MarketLimitButton = styled(WalletButton)`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  font-family: Poppins;
  font-size: 13px;
  margin-top: 4px;
  letter-spacing: 0.01em;
  padding: 0px;
  color: ${(p) => p.theme.color.interactive05};
`

export const Input = styled.input<{ hasError?: boolean }>`
  width: 100%;
  outline: none;
  background-image: none;
  background-color: ${leo.color.container.highlight};
  box-shadow: none;
  border: none;
  font-family: Poppins;
  font-size: 28px;
  line-height: 40px;
  letter-spacing: 0.02em;
  font-weight: 500;
  padding: 0px;
  margin: 4px 0px;
  color: ${(p) =>
    p.hasError ? p.theme.color.errorText : leo.color.text.primary};
  -webkit-box-shadow: none;
  -moz-box-shadow: none;
  ::placeholder {
    color: ${leo.color.text.tertiary};
  }
  :focus {
    outline: none;
  }
  ::-webkit-inner-spin-button {
    -webkit-appearance: none;
    margin: 0;
  }
  ::-webkit-outer-spin-button {
    -webkit-appearance: none;
    margin: 0;
  }
`
