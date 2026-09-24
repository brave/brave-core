// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import LeoButton from '@brave/leo/react/button'
import LeoButtonMenu from '@brave/leo/react/buttonMenu'

export const Button = styled(LeoButton)`
  --leo-button-color: ${leo.color.button.errorBackground};
`

export const ButtonMenu = styled(LeoButtonMenu)`
  --leo-menu-control-width: 900px;
  leo-menu-item {
    padding: 8px;
    display: flex;
    flex-direction: row;
    align-items: center;
    justify-content: flex-start;
    gap: 8px;
    color: ${leo.color.text.primary};
  }
  leo-menu-item:first-of-type {
    margin-top: 4px;
  }
  leo-menu-item:last-of-type {
    color: ${leo.color.systemfeedback.errorText};
  }
`

export const StyledMenuItem = styled
