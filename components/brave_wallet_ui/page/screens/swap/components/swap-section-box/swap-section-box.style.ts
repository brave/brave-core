// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import { StyledDiv } from '../shared-swap.styles'

export const Wrapper = styled(StyledDiv)<{ boxType: 'primary' | 'secondary' }>`
  --box-background-primary: ${leo.color.purple[10]};
  @media (prefers-color-scheme: dark) {
    --box-background-primary: ${(p) => p.theme.color.background02};
  }
  background-color: ${(p) =>
    p.boxType === 'primary'
      ? 'var(--box-background-primary)'
      : p.theme.color.background01};
  box-sizing: border-box;
  border-radius: 16px;
  border: ${(p) =>
    p.boxType === 'secondary'
      ? `1px solid ${p.theme.color.divider01}`
      : 'none'};
  height: ${(p) => (p.boxType === 'primary' ? '122px' : 'unset')};
  min-height: ${(p) => (p.boxType === 'secondary' ? '88px' : '114px')};
  padding: 14px 16px 14px 8px;
  width: 100%;
  position: relative;
  @media screen and (max-width: 570px) {
    min-height: ${(p) => (p.boxType === 'secondary' ? '114px' : '122px')};
  }
`
