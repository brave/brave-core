// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import LeoButtonMenu from '@brave/leo/react/buttonMenu'
import * as leo from '@brave/leo/tokens/css/variables'

export const ButtonMenu = styled(LeoButtonMenu)<{
  minWidth?: number
}>`
  color: ${leo.color.icon.default};
  leo-menu-item {
    --leo-icon-size: 18px;
    display: flex;
    align-items: center;
    justify-content: flex-start;
    gap: 16px;
    color: ${leo.color.text.primary};
    font: ${leo.font.default.regular};
    min-width: ${(p) => p.minWidth ?? 220}px;
    padding: 12px;
  }
  leo-menu-item #shield {
    --leo-icon-color: ${leo.color.systemfeedback.successIcon};
  }
  leo-menu-item#toggle {
    justify-content: space-between;
  }
`
