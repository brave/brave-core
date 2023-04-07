// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import { WalletButton } from '../../../shared/style'

export const Button = styled(WalletButton)`
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  outline: none;
  border: none;
  background: none;
  pointer-events: auto;
  border-right: 1px solid rgba(255,255,255,0.5);
  height: 100%;
  box-sizing: border-box;
  cursor: pointer;
  position: relative;
  &:last-child { 
    border-right: none;
  }
`

export const ButtonIcon = styled(Icon)`
  --leo-icon-size: 18px;
  color: ${(p) => p.theme.palette.white};
`
