// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { Column, WalletButton } from '$wallet/components/shared/style'

export const BoxContainer = styled(Column)`
  border: 1px solid ${leo.color.divider.subtle};
  border-radius: ${leo.radius.l};
  background-color: ${leo.color.container.background};
  overflow: hidden;
`

export const DetailsButton = styled(WalletButton)`
  cursor: pointer;
  display: flex;
  flex-direction: row;
  justify-content: space-between;
  align-items: center;
  padding: ${leo.spacing.m} ${leo.spacing.l};
  background: none;
  border: none;
  width: 100%;
  &:hover {
    background: ${leo.color.container.highlight};
  }
`

export const IconBubble = styled(Column)`
  --leo-icon-size: 20px;
  --leo-icon-color: ${leo.color.icon.default};
  width: 40px;
  height: 40px;
  border-radius: ${leo.radius.full};
  background-color: ${leo.color.container.highlight};
`
