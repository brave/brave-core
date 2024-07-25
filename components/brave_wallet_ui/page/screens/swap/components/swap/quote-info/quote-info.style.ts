// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import {
  WalletButton,
  Column,
  Row,
  Text //
} from '../../../../../../components/shared/style'
import { LPIcon } from '../../shared-swap.styles'

export const Bubble = styled(Row)`
  border-radius: 4px;
  background-color: ${leo.color.green[10]};
`

export const FreeText = styled(Text)`
  text-transform: uppercase;
  line-height: 10px;
  color: ${leo.color.green[60]};
  font-weight: 700;
`

export const LiquidityProviderIcon = styled(LPIcon)`
  border-radius: none;
`

export const BraveFeeDiscounted = styled(Text)`
  text-decoration: line-through;
`

export const Button = styled(WalletButton)`
  cursor: pointer;
  display: flex;
  flex-direction: row;
  background-color: none;
  background: none;
  outline: none;
  border: none;
  padding: 0px;
  margin: 0px;
  :disabled {
    cursor: default;
  }
`

export const ExpandButton = styled(Button)`
  background-color: ${leo.color.container.background};
  border-radius: 100%;
  padding: 1px;
`

export const Section = styled(Column)`
  background-color: ${leo.color.container.background};
  border-radius: ${leo.radius.m};
  position: relative;
`

export const CaratDownIcon = styled(Icon).attrs({
  name: 'carat-down'
})<{ isOpen: boolean }>`
  --leo-icon-size: 18px;
  color: ${leo.color.icon.default};
  transition-duration: 0.3s;
  transform: ${(p) => (p.isOpen ? 'rotate(180deg)' : 'unset')};
`

export const ExpandRow = styled(Row)`
  position: absolute;
  bottom: 0px;
  max-height: 0px;
`

export const CaratRightIcon = styled(Icon).attrs({
  name: 'carat-right'
})`
  --leo-icon-size: 18px;
  color: ${leo.color.icon.default};
`
