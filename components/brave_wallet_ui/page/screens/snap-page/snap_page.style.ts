// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Alert from '@brave/leo/react/alert'
import Button from '@brave/leo/react/button'

import { Column, Row, Text, WalletButton } from '../../../components/shared/style'

export const Page = styled(Column)`
  width: 100%;
  max-width: 800px;
  padding: 24px;
  color: ${leo.color.text.primary};
`

export const Header = styled(Column)`
  width: 100%;
  margin-bottom: 20px;
  gap: 8px;
`

export const BackLink = styled(WalletButton)`
  display: inline-flex;
  align-items: center;
  gap: 6px;
  background: none;
  border: none;
  padding: 0;
  cursor: pointer;
  font: ${leo.font.small.regular};
  color: ${leo.color.text.secondary};

  &:hover {
    color: ${leo.color.text.primary};
  }
`

export const TitleRow = styled(Row)`
  width: 100%;
  justify-content: space-between;
  align-items: center;
  gap: 12px;
`

export const SnapIdHeading = styled(Text)`
  margin: 0;
  font: ${leo.font.default.semibold};
  font-family: monospace;
  word-break: break-all;
`

export const HeaderActions = styled(Row)`
  gap: 8px;
  flex-shrink: 0;
`

export const UnloadButton = styled(Button)`
  --leo-button-color: ${leo.color.systemfeedback.errorText};
  --leo-button-background: transparent;
  --leo-button-border-color: ${leo.color.systemfeedback.errorText};
`

export const ErrorAlert = styled(Alert)`
  width: 100%;
  margin-bottom: 16px;
`

export const LoadingState = styled(Column)`
  width: 100%;
  align-items: center;
  justify-content: center;
  gap: 12px;
  padding: 32px;
`

export const LoadingText = styled(Text)`
  margin: 0;
  font: ${leo.font.small.regular};
  color: ${leo.color.text.tertiary};
`

export const ContentCard = styled.div`
  width: 100%;
  border: 1px solid ${leo.color.divider.subtle};
  border-radius: ${leo.radius.l};
  padding: 16px;
  background-color: ${leo.color.container.background};
  box-sizing: border-box;
`

export const EmptyState = styled(Column)`
  width: 100%;
  align-items: center;
  justify-content: center;
  padding: 32px;
`

export const EmptyStateText = styled(Text)`
  margin: 0;
  font: ${leo.font.small.regular};
  color: ${leo.color.text.tertiary};
  text-align: center;
`
