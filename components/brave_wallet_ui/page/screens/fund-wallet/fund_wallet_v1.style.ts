// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { layoutPanelWidth } from '../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper.style'

export const ControlPanel = styled.div`
  display: grid;
  grid-template-columns: 160px auto auto;
  gap: 16px;
  width: 100%;
  align-items: flex-start;

  @media (max-width: ${layoutPanelWidth}px) {
    grid-template-columns: 1fr;
  }
`
