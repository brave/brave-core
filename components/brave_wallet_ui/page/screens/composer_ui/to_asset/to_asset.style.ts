// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { Text, Row } from '../../../../components/shared/style'

export const ReceiveAndQuoteText = styled(Text)`
  line-height: 26px;
  color: ${leo.color.text.tertiary};
`

export const NetworkAndFiatText = styled(Text)`
  line-height: 22px;
  color: ${leo.color.text.tertiary};
`

export const ReceiveAndQuoteRow = styled(Row)`
  min-height: 26px;
`

export const SelectAndInputRow = styled(Row)`
  min-height: 60px;
`

export const NetworkAndFiatRow = styled(Row)`
  min-height: 22px;
`
