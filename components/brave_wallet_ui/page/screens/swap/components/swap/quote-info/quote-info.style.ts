// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import {
  Icon,
  Row,
  StyledDiv,
  Text,
  IconButton
} from '../../shared-swap.styles'

export const HorizontalArrows = styled(Icon)`
  color: ${(p) => p.theme.color.text03};
  margin-left: 8px;
`

export const FuelTank = styled(Icon)`
  color: ${(p) => p.theme.color.text02};
  margin-right: 6px;
`

export const Bubble = styled(Row)`
  padding: 2px 8px;
  border-radius: 8px;
  background-color: ${leo.color.purple[10]};
  @media (prefers-color-scheme: dark) {
    /* #282B37 does not exist in the design system */
    background-color: #282b37;
  }
`

export const LPIcon = styled(StyledDiv)<{ icon: string; size: number }>`
  background-image: url(${(p) => p.icon});
  background-size: cover;
  background-position: center;
  background-repeat: no-repeat;
  height: ${(p) => p.size}px;
  width: ${(p) => p.size}px;
  margin-left: 6px;
  border-radius: 50px;
`

export const LPSeparator = styled(Text)`
  padding: 0 6px;
`

export const BraveFeeContainer = styled(Row)`
  gap: 4px;
`

export const BraveFeeDiscounted = styled(Text)`
  text-decoration: line-through;
`

export const ExpandButton = styled(IconButton)<{
  isExpanded: boolean
}>`
  transform: ${(p) => (p.isExpanded ? 'rotate(180deg)' : 'unset')};
  transition: transform 300ms ease;
`

export const LPRow = styled(Row)`
  flex-wrap: wrap;
`
