// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// Assets
import BraveLogoLight from '../../../../send/assets/brave-logo-light.svg'
import BraveLogoDark from '../../../../send/assets/brave-logo-dark.svg'

import { StyledDiv } from '../../shared-swap.styles'

export const Wrapper = styled(StyledDiv)`
  display: flex;
  flex-direction: row;
  align-items: flex-start;
  justify-content: space-between;
  padding: 16px 32px 0px 32px;
  margin-bottom: 45px;
  top: 0;
  width: 100%;
  box-sizing: border-box;
  position: fixed;
  z-index: 10;
  @media screen and (max-width: 570px) {
    padding: 20px 16px 0px 16px;
    position: absolute;
  }
`

export const BraveLogo = styled(StyledDiv)`
  height: 30px;
  width: 100px;
  background-image: url(${BraveLogoLight});
  background-size: cover;
  margin: 0px 12px 4px 0px;
  @media (prefers-color-scheme: dark) {
    background-image: url(${BraveLogoDark});
  }
  @media screen and (max-width: 570px) {
    margin: 0px 0px 4px 0px;
  }
`

export const SelectorWrapper = styled(StyledDiv)`
  display: flex;
  position: relative;
`
