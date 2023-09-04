// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { StyledDiv } from '../../shared-swap.styles'


export const Wrapper = styled(StyledDiv)`
  background-color: ${p => p.theme.color.background01};
  border: 1px solid ${p => p.theme.color.disabled};
  border-radius: 4px;
  box-sizing: border-box;
  flex-direction: row;
  justify-content: center;
  padding: 4px 8px 4px 12px;
  width: 100%;
  min-height: 40px;
`

export const SelectorWrapper = styled(StyledDiv)`
  display: flex;
  position: relative;
`
