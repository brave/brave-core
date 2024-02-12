// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'

export const Wrapper = styled.span`
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 4px;
`

export const Rectangle = styled.span<{
  isActive: boolean
  width: number
}>`
  width: ${(p) => p.width}px;
  height: 8px;
  border-radius: 4px;
  background-color: ${(p) =>
    p.isActive ? leo.color.button.background : leo.color.purple[20]};
`
