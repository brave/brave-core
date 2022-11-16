// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// Shared Styles
import { StyledDiv, Column, Row } from '../../shared.styles'

export const Wrapper = styled(StyledDiv)`
  background-color: rgba(33, 37, 41, 0.32);
  backdrop-filter: blur(8px);
  bottom: 0%;
  left: 0%;
  position: absolute;
  right: 0%;
  top: 0%;
  z-index: 10;
`

export const Modal = styled(StyledDiv)`
  background-color: ${(p) => p.theme.color.background02};
  border-radius: 22px;
  box-shadow: 0px 0px 24px rgba(99, 105, 110, 0.36);
  box-sizing: border-box;
  flex-direction: column;
  height: 85%;
  justify-content: flex-start;
  overflow: hidden;
  position: absolute;
  width: 440px;
  z-index: 20;
  @media screen and (max-width: 570px) {
    width: 90%;
  }
  @media (prefers-color-scheme: dark) {
    box-shadow: 0px 0px 16px rgba(0, 0, 0, 0.36);
  }
`

export const AccountSection = styled(Row)`
  background-color: rgba(233, 233, 244, 0.3);
  @media (prefers-color-scheme: dark) {
    background-color: ${(p) => p.theme.color.background01};
  }
`

export const ScrollContainer = styled(Column)`
  flex: 1;
  overflow-x: hidden;
  overflow-y: auto;
`
