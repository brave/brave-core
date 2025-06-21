// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// Shared Styles
import {
  layoutPanelWidth, //
} from '../wallet-page-wrapper/wallet-page-wrapper.style'

export const MenuWrapper = styled.div`
  flex: 1;
  position: relative;
  display: flex;
  height: 100%;
`

export const Button = styled.button`
  flex: 1;
  display: flex;
  border: none;
  background: none;
  cursor: pointer;
  padding: 0;
  margin: 0;
  align-items: center;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    align-items: flex-start;
  }
`
