// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { color, font } from '@brave/leo/tokens/css/variables'

export const AccountName = styled.span`
  font: ${font.default.semibold};
  color: ${color.text.primary};
`

export const AccountAddress = styled.span`
  font: ${font.xSmall.regular};
  color: ${color.text.secondary};
`
