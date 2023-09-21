// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { Row } from '../../../shared/style'
import * as leo from '@brave/leo/tokens/css'
import { layoutPanelWidth } from '../../wallet-page-wrapper/wallet-page-wrapper.style'

export const ControlsRow = styled(Row)`
  box-shadow: 0px -1px 1px rgba(0, 0, 0, 0.02);
  border-radius: 16px 16px 0px 0px;
  padding: 24px 16px;
  background-color: ${leo.color.container.background};
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 16px;
  }
`

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  position: relative;
  height: 100%;
`
