// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Button from '@brave/leo/react/button'

// shared styles
import { Row } from '../shared/style'
import {
  layoutPanelWidth //
} from '../desktop/wallet-page-wrapper/wallet-page-wrapper.style'

export const ControlBarWrapper = styled(Row)<{
  showSearchBar: boolean
}>`
  padding: 0px 0px 0px 8px;
  margin-bottom: 16px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: ${(p) => (p.showSearchBar ? '0px' : '4px')} 0px 0px 8px;
    margin-bottom: ${(p) => (p.showSearchBar ? 12 : 16)}px;
  }
`

export const SearchBarWrapper = styled(Row)<{
  showSearchBar: boolean
}>`
  width: 230px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    display: ${(p) => (p.showSearchBar ? 'flex' : 'none')};
    width: 100%;
  }
`

export const BackButton = styled(Button).attrs({
  kind: 'outline',
  size: 'tiny'
})`
  --leo-button-radius: 8px;
  --leo-button-color: ${leo.color.text.interactive};
  margin-right: 12px;
  flex-grow: 0;
`
