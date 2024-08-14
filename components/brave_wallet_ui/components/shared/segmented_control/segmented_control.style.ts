// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import LeoSegmentedControl, {
  SegmentedControlProps
} from '@brave/leo/react/segmentedControl'

export const StyledWrapper = styled.div<{ width?: string }>`
  flex: 1;
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
`

export const SegmentedControl = styled(
  LeoSegmentedControl
)<SegmentedControlProps>`
  margin: 24px 0 10px 0;
`

export const ControlItemWrapper = styled.div`
  text-align: center;
`
