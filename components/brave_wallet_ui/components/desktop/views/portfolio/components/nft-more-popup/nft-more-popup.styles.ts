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
  border-radius: 8px;
  position: absolute;
  top: 42px;
  right: 12px;
  z-index: 3;
  border-radius: 8px;
  overflow: hidden;
  gap: 4px;
  padding: 4px;
  border: 1px solid ${leo.color.divider.subtle};
  background-color: ${leo.color.container.background};
  box-shadow: 0px 4px 16px -2px rgba(0, 0, 0, 0.08);
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
`

export const PopupButtonText = styled.span`
  flex: 1;
  font-size: 14px;
  font-style: normal;
  font-weight: 400;
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
