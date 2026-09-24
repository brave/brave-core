// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { Column } from '../../../../../components/shared/style'

export const Wrapper = styled(Column)`
  flex: 1;
  overflow-x: hidden;
  overflow-y: auto;
`

export const InfoColumn = styled(Column)`
  background-color: ${leo.color.systemfeedback.infoBackground};
`

export const InfoGraphic = styled.img`
  width: 100%;
  height: auto;
`

export const Link = styled.a`
  font: ${leo.font.small.semibold};
  color: ${leo.color.text.interactive};
  margin: 0px;
  padding: 0px;
  text-decoration: none;
  cursor: pointer;
`
