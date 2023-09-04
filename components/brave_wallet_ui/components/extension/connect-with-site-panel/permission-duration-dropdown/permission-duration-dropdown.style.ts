// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as leo from '@brave/leo/tokens/css'
import Icon from '@brave/leo/react/icon'
import styled from 'styled-components'
import { WalletButton } from '../../../shared/style'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  position: relative;
  width: 100%;
  box-sizing: border-box;
`

export const DropDownButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: space-between;
  width: 100%;
  flex-direction: row;
  background-color: ${leo.color.container.highlight};
  cursor: pointer;
  outline: none;
  border-radius: 8px;
  font-family: Poppins;
  font-style: normal;
  font-weight: 400;
  font-size: 14px;
  line-height: 24px;
  padding: 10px 8px 10px 16px;
  margin-bottom: 8px;
  color: ${leo.color.text.primary};
  border: none;
`

export const DropDownIcon = styled(Icon)<{ isOpen: boolean }>`
  --leo-icon-size: 20px;
  color: ${leo.color.icon.default};
  transition-duration: 0.3s;
  transform: ${(p) => (p.isOpen ? 'rotate(180deg)' : 'unset')};
`

export const DropDown = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  padding: 8px;
  box-sizing: border-box;
  background-color: ${leo.color.container.background};
  border: 1px solid ${leo.color.divider.subtle};
  border-radius: 8px;
  box-shadow: 0px 4px 13px -2px rgba(0, 0, 0, 0.08);
  @media (prefers-color-scheme: dark) {
    box-shadow: 0px 4px 13px -2px rgba(0, 0, 0, 0.35);
  }
  position: absolute;
  top: 48px;
  z-index: 9;
  @media screen and (max-width: 800px) {
    right: 0px;
  }
`

export const PermissionButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  flex-direction: row;
  width: 100%;
  cursor: pointer;
  outline: none;
  border: none;
  background: none;
  padding: 8px 8px;
  font-family: Poppins;
  font-style: normal;
  font-weight: 400;
  font-size: 14px;
  line-height: 24px;
  color: ${leo.color.text.primary};
  margin-bottom: 8px;
  &:last-child {
    margin-bottom: 0px;
  }
`
