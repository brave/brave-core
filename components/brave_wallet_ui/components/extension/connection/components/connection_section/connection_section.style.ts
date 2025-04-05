// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import LeoIcon from '@brave/leo/react/icon'

// Shared Styles
import { Column, Row, Text, WalletButton } from '../../../../shared/style'

export const ConnectionCard = styled(Column)<{
  connectionStatus: 'connected' | 'not-connected' | 'blocked'
}>`
  --connected-background: linear-gradient(
      180deg,
      rgba(203, 251, 192, 0.4) 0%,
      transparent 70%
    ),
    ${leo.color.page.background};
  --not-connected-background: linear-gradient(
      180deg,
      rgba(227, 227, 232, 0.4) 0%,
      transparent 70%
    ),
    ${leo.color.page.background};
  --blocked-background: linear-gradient(
      180deg,
      rgba(255, 218, 217, 0.6) 0%,
      rgba(255, 218, 217, 0.4) 70%
    ),
    ${leo.color.page.background};
  --connection-text-color: ${(p) =>
    p.connectionStatus === 'blocked'
      ? leo.color.red[60]
      : p.connectionStatus === 'connected'
      ? leo.color.green[60]
      : leo.color.neutral[60]};
  --card-border-blocked: linear-gradient(
    90deg,
    rgba(255, 188, 186, 0) 0%,
    #ffbcba 50.24%,
    rgba(255, 188, 186, 0) 100%
  );
  --card-border-connected: linear-gradient(
    90deg,
    rgba(145, 219, 144, 0) 0%,
    #91db90 50.24%,
    rgba(145, 219, 144, 0) 100%
  );
  --card-border: ${(p) =>
    p.connectionStatus === 'blocked'
      ? 'var(--card-border-blocked)'
      : p.connectionStatus === 'connected'
      ? 'var(--card-border-connected)'
      : 'transparent'};
  background: ${(p) =>
    p.connectionStatus === 'blocked'
      ? 'var(--blocked-background)'
      : p.connectionStatus === 'connected'
      ? 'var(--connected-background)'
      : 'var(--not-connected-background)'};
  border-radius: ${leo.radius.xl};
  overflow: hidden;
`

export const TopCardBorder = styled(Row)`
  height: 2px;
  background: var(--card-border);
`

export const ControlsWrapper = styled(Column)`
  background-color: ${leo.color.container.background};
  border-radius: ${leo.radius.l};
  box-shadow: 0px var(--Elevation-xxs, 1px) 0px 0px
      var(--Semantic-Elevation-Primary, rgba(0, 0, 0, 0.05)),
    0px var(--Elevation-xxs, 1px) var(--Elevation-xs, 4px) 0px
      var(--Semantic-Elevation-Secondary, rgba(0, 0, 0, 0.1));
  box-shadow: ${leo.elevation.l};
`

export const Icon = styled(LeoIcon)`
  --leo-icon-color: var(--connection-text-color);
  --leo-icon-size: 12px;
  margin-right: 6px;
`

export const StatusText = styled(Text)`
  text-transform: uppercase;
  font: ${leo.font.default.semibold};
  font-size: 10px;
  font-weight: 700;
  line-height: 14px;
  color: var(--connection-text-color);
`

export const SelectButton = styled(WalletButton)`
  cursor: pointer;
  width: 100%;
  border: none;
  background: none;
  padding: 12px;
  display: flex;
  flex-direction: row;
  justify-content: space-between;
  align-items: center;
  border-radius: 8px;
  &:hover {
    background-color: ${leo.color.container.highlight};
  }
`

export const SelectButtonIcon = styled(LeoIcon).attrs({
  name: 'arrow-up-and-down'
})`
  --leo-icon-color: ${leo.color.icon.default};
  --leo-icon-size: 18px;
  margin-left: 12px;
`
