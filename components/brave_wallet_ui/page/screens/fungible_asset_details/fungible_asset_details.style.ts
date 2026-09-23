// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  height: 100%;
`

export const ButtonRow = styled.div<{
  noMargin?: boolean
  horizontalPadding?: number
}>`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  margin: ${(p) => (p.noMargin ? '0px' : '20px 0px')};
  padding: 0px
    ${(p) => (p.horizontalPadding !== undefined ? p.horizontalPadding : 0)}px;
  gap: 12px;
`
