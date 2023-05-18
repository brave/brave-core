// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import Icon from '@brave/leo/react/icon'
import { WalletButton } from '../../../../../shared/style'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background-color: ${(p) => p.theme.color.background02};
  border-radius: 8px;
  box-shadow: 0px 4px 13px -2px rgba(0, 0, 0, 0.1);
  position: absolute;
  top: 42px;
  right: 12px;
  z-index: 3;
  width: 127px;
  border: 1px solid ${leo.color.divider.subtle};
  border-radius: 8px;
  overflow: hidden;
`

export const PopupButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  text-align: left;
  cursor: pointer;
  width: 100%;
  outline: none;
  border: none;
  background: none;
  padding: 9px 8px;
  margin: 0px;
  background-color: transparent;
  &:hover {
    background-color: ${(p) => p.theme.color.divider01};
  }
`

export const PopupButtonText = styled.span`
  flex: 1;
  font-family: Poppins;
  font-size: 13px;
  font-weight: 400;
  letter-spacing: 0.01em;
  line-height: 24px;
  color: ${leo.color.text.primary};
  white-space: nowrap;
  text-overflow: ellipsis;
  overflow: hidden;
  width: 90%;
`

export const ButtonIcon = styled(Icon)`
  --leo-icon-size: 18px;
  color: ${leo.color.icon.default};
  margin-right: 8px;
`
