// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import LeoSegmentedControl, {
  SegmentedControlProps
} from '@brave/leo/react/segmentedControl'

// Shared Styles
import {
  layoutPanelWidth,
  layoutSmallWidth //
} from '../../desktop/wallet-page-wrapper/wallet-page-wrapper.style'

export const StyledWrapper = styled.div<{ maxWidth?: string }>`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  z-index: 0;
  width: 100%;
  max-width: ${(p) => p.maxWidth ?? 'unset'};
  --leo-segmented-control-width: 100%;
  @media screen and (max-width: ${layoutSmallWidth}px) {
    max-width: unset;
  }
`

export const SegmentedControl = styled(LeoSegmentedControl).attrs({
  size: window.innerWidth <= layoutPanelWidth ? 'small' : 'default'
})<SegmentedControlProps>``

export const ControlItemWrapper = styled.div`
  text-align: center;
`
