// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { Text, Row } from '../../../../components/shared/style'

export const InputRow = styled(Row)`
  box-sizing: border-box;
  position: relative;
`

export const ToText = styled(Text)`
  line-height: 26px;
  color: ${leo.color.text.tertiary};
`

export const ToRow = styled(Row)`
  min-height: 26px;
`
