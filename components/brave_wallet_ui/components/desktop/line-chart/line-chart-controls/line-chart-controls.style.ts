// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import { WalletButton } from '../../../shared/style'

export const SelectTimelineClickArea = styled.div`
  position: relative;
`

export const SelectTimelinButton = styled(WalletButton)`
  --button-border: ${leo.color.primary[20]};
  @media (prefers-color-scheme: dark) {
    --button-border: ${leo.color.primary[50]};
  }
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  border-radius: 8px;
  padding: 6px 10px 6px 18px;
  border: 1px solid var(--button-border);
  font-family: Poppins;
  font-size: 12px;
  line-height: 16px;
  letter-spacing: 0.03em;
  font-weight: 600;
  color: ${leo.color.text.interactive};
`

export const SelectTimelinButtonIcon = styled(Icon)<{
  isOpen: boolean
}>`
  --leo-icon-size: 16px;
  color: ${leo.color.icon.interactive};
  margin-left: 8px;
  transition-duration: 0.3s;
  transform: ${(p) => (p.isOpen ? 'rotate(180deg)' : 'unset')};
`
