/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import {
  AssetIconProps,
  AssetIconFactory,
  WalletButton
} from '../../shared/style'

export const StyledWrapper = styled(WalletButton)`
  display: flex;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  margin-bottom: 10px;
  padding: 8px;
  border-radius: 8px;
  border: 1px solid ${leo.color.divider.subtle};

  &:hover {
    border-color: ${leo.color.button.background};
    background-color: ${leo.color.container.interactive};
  }
`

export const CurrencyNameAndCode = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: center;
  flex-direction: column;
`

export const CurrencyName = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 600;
  color: ${leo.color.text.primary};
  text-align: left;
`

export const CurrencySymbol = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${leo.color.text.secondary};
`

// Construct styled-component using JS object instead of string, for editor
// support with custom AssetIconFactory.
//
// Ref: https://styled-components.com/docs/advanced#style-objects
export const AssetIcon = AssetIconFactory<AssetIconProps>({
  width: '24px',
  height: 'auto'
})
