// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { IconButton } from '../../shared-swap.styles'

export const MoreOptionsButton = styled(IconButton) <{
  isSelected: boolean
}>`
  transform: ${(p) => (p.isSelected ? 'rotate(180deg)' : 'unset')};
`
