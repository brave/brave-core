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
  font-family: 'Poppins';
  font-weight: 600;
  font-size: 10px;
  line-height: 15px;
  text-transform: uppercase;
  letter-spacing: 0.4px;
`

export const NetworkName = styled.span`
  font-family: 'Poppins';
  color: ${leo.color.text.primary};
  line-height: 24px;
  word-wrap: wrap;
  word-break: break-all;
  text-align: left;
  font-size: 14px;
  font-weight: 400;
  letter-spacing: 0.02em;
`
