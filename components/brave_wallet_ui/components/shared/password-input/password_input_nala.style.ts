// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Input from '@brave/leo/react/input'
import Icon from '@brave/leo/react/icon'
import * as leo from '@brave/leo/tokens/css/variables'

export const FullWidthInput = styled(Input)`
  width: 100%;
`

export const LockIcon = styled(Icon).attrs({ name: 'lock-plain' })`
  --leo-icon-size: 20px;
  color: ${leo.color.icon.default};
`
