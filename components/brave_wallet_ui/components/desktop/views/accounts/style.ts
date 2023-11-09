// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'

import { Column } from '../../../shared/style'
import { layoutPanelWidth } from '../../wallet-page-wrapper/wallet-page-wrapper.style'

export const ControlsWrapper = styled(Column)`
  padding: 0px 32px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 0px 16px;
  }
`

export const AssetsWrapper = styled(Column)`
  padding: 16px 24px 24px 24px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 16px;
  }
`

export const NFTsWrapper = styled(Column)`
  padding: 16px 32px 24px 32px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 16px;
  }
`

export const TransactionsWrapper = styled(Column)`
  padding: 16px 24px 24px 24px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 16px;
  }
`

export const SectionTitle = styled.span`
  font-family: Poppins;
  font-size: 14px;
  line-height: 24px;
  font-weight: 600;
  color: ${leo.color.text.secondary};
`
