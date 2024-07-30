// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import LeoSegmentedControl from '@brave/leo/react/segmentedControl'
import { layoutPanelWidth } from '../../wallet-page-wrapper/wallet-page-wrapper.style'

export const SegmentedControl = styled(LeoSegmentedControl)`
  --control-height: 60px;
  margin: 24px 0 10px 0;
`

export const ControlItemWrapper = styled.div`
  @media screen and (min-width: ${layoutPanelWidth}px) {
    padding: 0 55px;
  }
`
