// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import ProgressRing from '@brave/leo/react/progressRing'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { Text } from '../../../../components/shared/style'

export const LoadingIcon = styled(ProgressRing)`
  --leo-progressring-size: 40px;
  --leo-progressring-color: ${leo.color.icon.interactive};
  margin-bottom: 20px;
`

export const CreatingWalletText = styled(Text)`
  font-weight: 500;
  line-height: 28px;
  color: ${leo.color.text.primary};
`
