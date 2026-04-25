// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { color, spacing } from '@brave/leo/tokens/css/variables'
import styled from 'styled-components'

const Card = styled.div`
  background-color: ${color.container.background};
  border: none;
  overflow: hidden;
  padding: ${spacing.l} ${spacing['2Xl']};
`

export default Card
