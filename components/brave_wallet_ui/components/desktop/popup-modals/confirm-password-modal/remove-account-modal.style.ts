// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// style
import { layoutPanelWidth } from '../../wallet-page-wrapper/wallet-page-wrapper.style'

export const modalWidth = '442px'

export const StyledWrapper = styled.div`
  width: 100%;
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: center;
  padding: 0px ${leo.spacing['3Xl']} ${leo.spacing['3Xl']} ${leo.spacing['3Xl']};
  @media screen and (max-width: ${layoutPanelWidth}px) {
    height: 100%;
  }
`
