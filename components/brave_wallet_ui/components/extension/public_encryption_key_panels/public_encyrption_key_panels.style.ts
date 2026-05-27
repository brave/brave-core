// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { Column, Text } from '../../shared/style'

export const StyledWrapper = styled(Column)`
  background-color: ${leo.color.page.background};
`

export const HeaderText = styled(Text)`
  font: ${leo.font.heading.h4};
`

export const AccountNameText = styled(Text)`
  font: ${leo.font.heading.h4};
`

export const Title = styled(Text)`
  font: ${leo.font.heading.h3};
`

export const MessageContainer = styled(Column)`
  background-color: ${leo.color.container.highlight};
  border-radius: ${leo.radius.l};
`

export const MessageContainerTitle = styled(Text)`
  font: ${leo.font.default.semibold};
`

export const MessageBox = styled(Column)`
  background-color: ${leo.color.container.background};
  box-sizing: border-box;
  border-radius: 10px;
  min-height: 100px;
  max-height: 140px;
  overflow-x: hidden;
  overflow-y: scroll;
`

export const MessageText = styled(Text)`
  font: ${leo.font.default.regular};
  flex-wrap: wrap;
  word-break: break-word;
`

export const DecryptMessageBox = styled(Column)`
  background-color: ${leo.color.container.interactive};
  border-radius: ${leo.radius.l};
  height: 166px;
`

export const URLText = styled(Text)`
  font: ${leo.font.default.regular};
  max-width: 80%;
  word-break: break-word;
`
