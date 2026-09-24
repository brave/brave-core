// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as leo from '@brave/leo/tokens/css/variables'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import styled from 'styled-components'

// Shared Styles
import {
  Text,
  Column,
  Row,
  WalletButton,
} from '$wallet/components/shared/style'

export const Header = styled(Row)`
  display: grid;
  grid-template-columns: 1fr auto 1fr;

  &::after {
    content: '';
  }

  & > :first-child {
    justify-self: start;
  }
`

export const BackButton = styled(Button)`
  flex-grow: 0;
`

export const BackIcon = styled(Icon)`
  --leo-icon-color: ${leo.color.icon.default};
`

export const SectionLabel = styled(Text)`
  text-transform: uppercase;
  padding: 0px 8px;
  margin-bottom: 4px;
`

export const IconBubble = styled(Column)`
  --leo-icon-color: ${leo.color.icon.default};
  --leo-icon-size: 14px;
  width: 28px;
  height: 28px;
  border-radius: ${leo.radius.full};
  background-color: ${leo.color.container.highlight};
`

export const SectionButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  flex-direction: row;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  padding: 12px;
  width: 100%;
  border-radius: ${leo.radius.m};
  gap: 8px;
  &:hover {
    background-color: ${leo.color.container.highlight};
  }
`
