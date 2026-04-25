// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Dropdown from '@brave/leo/react/dropdown'

// Shared Styles
import { Row } from '../style'

export const DropdownOption = styled(Row)<{
  isDisabled?: boolean
}>`
  opacity: ${(p) => (p.isDisabled ? 0.6 : 1)};
`

export const DropdownFilter = styled(Dropdown)`
  width: 100%;
  color: ${leo.color.text.primary};
`
