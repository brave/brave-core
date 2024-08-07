// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Input from '@brave/leo/react/input'

// Shared Styles
import { Column, Text } from '../../../../components/shared/style'

export const ScrollContainer = styled(Column)`
  flex: 0.98;
  overflow-x: hidden;
  overflow-y: auto;
`

export const NoAssetsText = styled(Text)`
  color: ${leo.color.text.tertiary};
  line-height: 22px;
`

export const SearchInput = styled(Input)`
  width: 100%;
`
