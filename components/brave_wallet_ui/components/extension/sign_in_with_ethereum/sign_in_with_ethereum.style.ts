// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import {
  Text,
  WalletButton,
  Column,
  ScrollableColumn,
} from '../../shared/style'

export const StyledWrapper = styled(Column)`
  background-color: ${leo.color.page.background};
`

export const HeaderText = styled(Text)`
  font: ${leo.font.heading.h4};
  letter-spacing: ${leo.typography.letterSpacing.large};
`

export const MessageBox = styled(Column)`
  max-height: 280px;
  background-color: ${leo.color.container.background};
  border-radius: 12px;
  overflow-x: hidden;
  overflow-y: auto;
`

export const Title = styled(Text)`
  font: ${leo.font.heading.h4};
  letter-spacing: ${leo.typography.letterSpacing.large};
`

export const MessageText = styled(Text)`
  font: ${leo.font.default.regular};
  letter-spacing: ${leo.typography.letterSpacing.default};
  text-align: left;
  flex-wrap: wrap;
`

export const SectionTitle = styled(Text)`
  font: ${leo.font.default.semibold};
  letter-spacing: ${leo.typography.letterSpacing.default};
`

export const URLText = styled(MessageText)`
  word-break: break-all;
`

export const DetailsWrapper = styled(Column)`
  background-color: ${leo.color.container.background};
  box-sizing: border-box;
  overflow: hidden;
`

export const DetailsTitle = styled(Title)`
  font: ${leo.font.heading.h2};
  letter-spacing: ${leo.typography.letterSpacing.headings};
`

export const DetailsContent = styled(Column)`
  overflow: hidden;
  box-sizing: border-box;
`

export const DetailsBox = styled(ScrollableColumn)`
  background-color: ${leo.color.container.highlight};
  border-radius: ${leo.radius.m};
  overflow-x: hidden;
`

export const DetailsKeyText = styled(Text)`
  font: ${leo.font.default.regular};
  letter-spacing: ${leo.typography.letterSpacing.default};
  text-align: left;
  flex-wrap: wrap;
`

export const CodeBlock = styled.code``

export const DetailsInfoText = styled(Text)`
  font: ${leo.font.default.regular};
  letter-spacing: ${leo.typography.letterSpacing.default};
  text-align: left;
  flex-wrap: wrap;
  word-wrap: break-word;
  max-width: 100%;
`

export const IconButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  border: none;
  background: none;
  padding: 0px;
  margin: 0px;
  background-color: transparent;
`

export const CloseIcon = styled(Icon).attrs({
  name: 'close',
})`
  --leo-icon-size: 24px;
  color: ${leo.color.icon.default};
`
