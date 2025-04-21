// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import LeoSegmentedControl, {
  SegmentedControlProps
} from '@brave/leo/react/segmentedControl'

// Shared Styles
import { Title } from '../onboarding/onboarding.style'
import { Row, Text } from '../../../components/shared/style'

export const DepositTitle = styled(Title)`
  margin-top: 0px;
`

export const ControlsWrapper = styled(Row)`
  margin-bottom: 24px;
  --leo-segmented-control-width: 100%;
`

export const SegmentedControl = styled(LeoSegmentedControl).attrs({
  size: 'default'
})<SegmentedControlProps>``

export const AddressText = styled(Text)`
  word-break: break-all;
  max-width: 500px;
`
