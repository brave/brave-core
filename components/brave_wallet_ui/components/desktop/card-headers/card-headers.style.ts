// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import Icon from '@brave/leo/react/icon'
import { WalletButton } from '../../shared/style'

export const HeaderTitle = styled.span`
  font-family: Poppins;
  font-style: normal;
  font-size: 28px;
  font-weight: 500;
  line-height: 40px;
  color: ${leo.color.text.primary};
`

export const MenuWrapper = styled.div`
  position: relative;
`

export const CircleButton = styled(WalletButton)`
  --button-border-color: ${leo.color.primary[20]};
  @media (prefers-color-scheme: dark) {
    --button-border-color: ${leo.color.primary[50]};
  }
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  background-color: ${leo.color.container.background};
  border-radius: 100%;
  border: 1px solid var(--button-border-color);
  height: 36px;
  width: 36px;
`

export const ButtonIcon = styled(Icon)`
  --leo-icon-size: 18px;
  color: ${leo.color.icon.interactive};
`
