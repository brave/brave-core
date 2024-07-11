// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Styles
import { WalletButton } from '../../../shared/style'

export const StyledWrapper = styled(WalletButton)`
  box-sizing: border-box;
  position: relative;
  display: flex;
  flex-direction: row;
  justify-content: flex-start;
  gap: 8px;
  width: 100%;
  padding: 12px 8px;
  border-radius: 8px;
  cursor: pointer;
  outline: none;
  border: none;
  background-color: ${leo.color.container.background};

  &:hover {
    background-color: ${leo.color.container.highlight};
  }
`

export const PlaceholderImage = styled(Icon).attrs({
  name: 'image'
})<{ size?: string }>`
  --leo-icon-size: ${(p) => p.size ?? '40px'};
`
