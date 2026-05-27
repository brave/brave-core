// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import { WalletButton } from '../../../../../shared/style'

export const NetworkButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  cursor: pointer;
  outline: none;
  border: none;
  background: none;
  padding: 12px;
  background-color: transparent;
  width: 100%;
  border-radius: 8px;
  &:hover {
    background-color: ${leo.color.container.highlight};
  }
`

export const ActiveIndicator = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  border-radius: 4px;
  padding: 2px 6px;
  background-color: ${leo.color.primary[20]};
  color: ${leo.color.primary[50]};
  font-weight: 600;
  font: ${leo.font.xSmall.regular};
  text-transform: uppercase;
`

export const NetworkName = styled.span`
  color: ${leo.color.text.primary};
  word-wrap: wrap;
  word-break: break-all;
  text-align: left;
  font: ${leo.font.default.regular};
`
