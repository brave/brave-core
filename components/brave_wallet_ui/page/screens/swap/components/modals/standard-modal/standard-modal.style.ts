// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { StyledDiv } from '../../shared-swap.styles'

export const Wrapper = styled(StyledDiv)`
  /* This RGBA value does not exist in the design system */
  background-color: rgba(196, 196, 196, 0.3);
  @media (prefers-color-scheme: dark) {
    /* This RGBA value does not exist in the design system */
    background-color: rgba(47, 47, 47, 0.46);
  }
  bottom: 0%;
  left: 0%;
  position: absolute;
  right: 0%;
  top: 0%;
  z-index: 10;
`

export const Modal = styled(StyledDiv)<{
  modalHeight?: 'standard' | 'full' | 'dynamic'
  modalBackground?: 'background01' | 'background02'
}>`
  background-color: ${(p) =>
    p.modalBackground === 'background02'
      ? p.theme.color.background02
      : p.theme.color.background01};
  border-radius: 22px;
  box-shadow: 0px 0px 24px rgba(99, 105, 110, 0.36);
  box-sizing: border-box;
  flex-direction: column;
  height: ${(p) =>
    p.modalHeight === 'full'
      ? '85%'
      : p.modalHeight === 'dynamic'
      ? 'unset'
      : '520px'};
  justify-content: flex-start;
  overflow: hidden;
  position: absolute;
  width: 440px;
  z-index: 20;
  @media (prefers-color-scheme: dark) {
    box-shadow: 0px 0px 16px rgba(0, 0, 0, 0.36);
  }
  @media screen and (max-width: 570px) {
    position: fixed;
    right: 0px;
    left: 0px;
    top: 72px;
    bottom: 0px;
    width: auto;
    height: auto;
    border-radius: 16px 16px 0px 0px;
  }
`
