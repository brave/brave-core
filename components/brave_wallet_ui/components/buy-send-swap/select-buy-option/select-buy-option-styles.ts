/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled, { css } from 'styled-components'

export const StyledWrapper = styled.div<{
  layoutType?: 'loose' | 'tight'
}>`
  display: flex;
  flex-direction: column;
  align-items: flex-start;

  ${(p) => p?.layoutType === 'loose'
    ? css`
      width: 100%;
      align-items: stretch;
    `
    : ''
  }
`

export const SubDivider = styled.div`
  width: 100%;
  height: 1px;
  background-color: ${(p) => p.theme.color.divider01};
`
