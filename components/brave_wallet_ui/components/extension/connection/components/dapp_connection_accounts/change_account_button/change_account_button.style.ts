// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import { WalletButton } from '../../../../../shared/style'

export const AccountButton = styled(WalletButton)`
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
