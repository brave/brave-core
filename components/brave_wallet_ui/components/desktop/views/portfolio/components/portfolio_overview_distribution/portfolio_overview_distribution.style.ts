// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { Row, Text, Column } from '../../../../../shared/style'

export const Title = styled(Text)`
  font: ${leo.font.small.regular};
  letter-spacing: ${leo.typography.letterSpacing.default};
`

// Named container so fiat can hide when the flex slice is too narrow (panel / small widths).
const distributionSegmentContainerName = 'distribution-segment'

export const Segment = styled(Column)<{ $grow: number }>`
  flex: ${(p) => p.$grow} 1 0;
  min-width: 0;
  overflow: hidden;
  container-type: inline-size;
  container-name: ${distributionSegmentContainerName};
`

export const SegmentBar = styled.div<{ $color: string }>`
  width: 100%;
  height: 8px;
  border-radius: ${leo.radius.xl};
  background-color: ${(p) => p.$color};
`

export const SegmentMetaRow = styled(Row)`
  min-width: 0;
  width: 100%;
  overflow: hidden;
`

export const SegmentSymbol = styled(Title)`
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
`

export const SegmentPercent = styled(Title)`
  flex-shrink: 0;
`

// ~12px figures: hide fiat below this inline size so labels do not collide in narrow flex slots.
const minSegmentWidthPxToShowFiat = 104

export const SegmentFiat = styled(Text)`
  width: 100%;
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;

  @container ${distributionSegmentContainerName} (max-width: ${minSegmentWidthPxToShowFiat}px) {
    display: none;
  }
`
