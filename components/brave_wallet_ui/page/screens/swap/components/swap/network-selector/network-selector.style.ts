// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { StyledDiv, Row } from '../../shared-swap.styles'

export const SelectorBox = styled(StyledDiv)<{
  isHeader?: boolean
}>`
  justify-content: flex-start;
  background-color: ${(p) => p.theme.color.background01};
  min-width: 222px;
  position: absolute;
  padding-bottom: 4px;
  z-index: 10;
  top: ${(p) => (p.isHeader ? 42 : 40)}px;
  box-shadow: 0px 0px 16px rgba(99, 105, 110, 0.18);
  right: ${(p) => (p.isHeader ? 'unset' : '-10px')};
  border-radius: ${(p) => (p.isHeader ? 16 : 4)}px;
  @media (prefers-color-scheme: dark) {
    box-shadow: 0px 0px 16px rgba(0, 0, 0, 0.36);
  }
  @media screen and (max-width: 800px) {
    right: ${(p) => (p.isHeader ? '0px' : 'unset')};
  }
  @media screen and (max-width: 570px) {
    position: ${(p) => (p.isHeader ? 'fixed' : 'absolute')};
    right: ${(p) => (p.isHeader ? '0px' : '-20px')};
    left: ${(p) => (p.isHeader ? '0px' : 'unset')};
    top: ${(p) => (p.isHeader ? '72px' : '40px')};
    bottom: ${(p) => (p.isHeader ? '0px' : 'unset')};
    width: ${(p) => (p.isHeader ? 'auto' : '90vw')};
    border-radius: ${(p) => (p.isHeader ? '16px 16px 0px 0px' : '4px')};
  }
`

export const HeaderRow = styled(Row)<{
  isHeader?: boolean
}>`
  display: none;
  padding-top: 24px;
  @media screen and (max-width: 570px) {
    display: ${(p) => (p.isHeader ? 'flex' : 'none')};
  }
`
