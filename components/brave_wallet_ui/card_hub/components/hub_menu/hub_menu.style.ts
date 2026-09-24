// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as leo from '@brave/leo/tokens/css/variables'
import LeoButtonMenu from '@brave/leo/react/buttonMenu'
import styled from 'styled-components'

export const ButtonMenu = styled(LeoButtonMenu)`
  leo-menu-item {
    --leo-icon-color: ${leo.color.icon.default};
    --leo-icon-size: 20px;
    display: flex;
    align-items: center;
    gap: ${leo.spacing.l};
    padding: ${leo.spacing.m};
    min-width: 200px;
    color: ${leo.color.text.primary};
    font: ${leo.font.default.regular};
  }
`
