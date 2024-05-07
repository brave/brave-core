// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import { Text } from '../../../../../components/shared/style'

export const PercentChangeText = styled(Text)<{
  isDown: boolean
}>`
  color: ${(p) =>
    p.isDown
      ? leo.color.systemfeedback.errorText
      : leo.color.systemfeedback.successText};
`

export const CopyIcon = styled(Icon).attrs({
  name: 'copy'
})`
  --leo-icon-size: 16px;
  color: ${leo.color.icon.default};
`
