// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as leo from '@brave/leo/tokens/css/variables'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import styled from 'styled-components'

// Shared Styles
import { Row } from '$wallet/components/shared/style'

export const HeaderWrapper = styled(Row)`
  display: grid;
  grid-template-columns: 1fr auto 1fr;

  &::after {
    content: '';
  }

  & > :first-child {
    justify-self: start;
  }
`

export const BackButton = styled(Button)`
  flex-grow: 0;
`

export const BackIcon = styled(Icon)`
  --leo-icon-color: ${leo.color.icon.default};
`
