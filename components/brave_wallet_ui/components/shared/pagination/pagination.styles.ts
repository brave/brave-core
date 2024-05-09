// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Button from '@brave/leo/react/button'

import { Row } from '../style'

export const PaginationRow = styled(Row)`
  gap: ${leo.spacing.s};
`

export const PaginationButton = styled(Button)`
  --leo-button-padding: ${leo.spacing.s};
  min-width: 36px;
  flex: 1;
`
