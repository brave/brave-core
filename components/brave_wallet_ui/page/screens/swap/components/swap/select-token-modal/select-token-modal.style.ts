// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { Column } from '../../shared-swap.styles'
import {
  StandardButton
} from '../../buttons/standard-button/standard-button'

export const Button = styled(StandardButton)`
  align-self: flex-end;
  margin: auto;
`

export const ScrollContainer = styled(Column)`
  flex: 1;
  overflow: hidden;
  @media screen and (max-width: 570px) {
    padding: 0;
  }
`
