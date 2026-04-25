// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'
import { WalletButton, Row } from '../../shared/style'

export const Button = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  padding: 0px;
  margin: 0px;
`

export const ButtonIcon = styled(Icon)`
  --leo-icon-size: 24px;
  color: ${leo.color.icon.default};
`

export const LeftRightContainer = styled(Row)`
  min-width: 35%;
`

export const ClickAwayArea = styled.div`
  left: 0;
  right: 0;
  top: 0;
  bottom: 0;
  position: fixed;
  z-index: 7;
`
