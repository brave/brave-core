// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import LeoSegmentedControl from '@brave/leo/react/segmentedControl'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  position: relative;
  height: 100%;
`

export const SegmentedControl = styled(LeoSegmentedControl)`
  --leo-control-item-padding: 16px 50px;
  margin: 24px 32px;
`
