// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'

// Shared Styles
import {
  StyledButton
} from '../../../page/screens/send/shared.styles'

export const ButtonsContainer = styled.div<{
  width?: number;
}>`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  box-sizing: border-box;
  overflow: hidden;
  padding: 4px;
  width: ${(p) =>
    p.width !== undefined
      ? `${p.width}px`
      : '100%'
  };
  height: 48px;
  background-color: ${leo.color.container.highlight};
  border-radius: 100px;
`

export const Button = styled(StyledButton) <{
  isSelected: boolean
}>`
  --selected-background-color: ${leo.color.container.background};
  @media (prefers-color-scheme: dark) {
    --selected-background-color: ${leo.color.gray[20]};
  }
  background-color: ${(p) =>
    p.isSelected
      ? 'var(--selected-background-color)'
      : 'none'
  };
  border-radius: 100px;
  font-weight: 600;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.03em;
  width: 100%;
  padding: 10px;
  color: ${(p) =>
    p.isSelected
      ? leo.color.text.primary
      : leo.color.text.secondary
  };
`
