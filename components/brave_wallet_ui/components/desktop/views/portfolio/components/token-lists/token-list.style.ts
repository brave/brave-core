// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// Shared Styles
import { Column } from '../../../../../shared/style'
import {
  layoutPanelWidth //
} from '../../../../wallet-page-wrapper/wallet-page-wrapper.style'

export const listItemInitialHeight = 76

export const AutoSizerStyle: React.CSSProperties = {
  width: '100%',
  height: '100%'
}

export const FlatTokenListWrapper = styled(Column)`
  padding: 0px 20px 20px 20px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 0px 8px 8px 8px;
  }
`

export const GroupTokenListWrapper = styled(Column)`
  padding: 0px 32px 32px 32px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 0px 16px 16px 16px;
  }
`
