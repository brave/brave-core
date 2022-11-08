// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// icons
import { CaratCircleODownIcon } from 'brave-ui/components/icons'

// style
import { WalletButton } from '../style'

interface IsPanelProps {
  isPanel?: boolean
}

export const CaratDownIcon = styled(CaratCircleODownIcon)<IsPanelProps>`
  width: 14px;
  height: 14px;
  margin-left: 4px;
  color: ${(p) => p.isPanel ? p.theme.palette.white : p.theme.color.interactive07};
`

export const OvalButton = styled(WalletButton)<IsPanelProps>`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: ${(p) => p.onClick ? 'pointer' : 'text'};
  outline: none;
  background: none;
  border-radius: 48px;
  padding: 3px 10px;
  border: 1px solid ${(p) => p.isPanel ? 'rgba(255,255,255,0.5)' : p.theme.color.interactive08};
`

export const OvalButtonText = styled.span<IsPanelProps>`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.isPanel ? p.theme.palette.white : p.theme.color.text02};
  font-weight: 600;
`
