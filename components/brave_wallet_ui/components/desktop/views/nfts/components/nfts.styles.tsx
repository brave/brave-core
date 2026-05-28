// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as leo from '@brave/leo/tokens/css/variables'
import styled from 'styled-components'

// Shared Styles
import { Row, ScrollableColumn } from '../../../../shared/style'
import {
  layoutPanelWidth, //
} from '../../../wallet-page-wrapper/wallet-page-wrapper.style'

export const NftGrid = styled.div<{
  padding?: string
}>`
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  grid-gap: 16px;
  box-sizing: border-box;
  width: 100%;
  padding: ${(p) => p.padding ?? '16px'};
  @media screen and (max-width: ${layoutPanelWidth}px) {
    grid-template-columns: repeat(2, 1fr);
    grid-gap: 8px;
    padding: ${(p) => p.padding ?? '8px'};
  }
  & > * {
    min-height: 250px;
  }
`

export const EmptyStateText = styled.div`
  font: ${leo.font.default.regular};

  text-align: center;
  padding: 30px 0;
  color: ${(p) => p.theme.color.text03};
`

export const BannerWrapper = styled(Row)`
  padding: 0px 32px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 0px 16px;
  }
`

export const NFTListWrapper = styled(ScrollableColumn)`
  padding: 0px 32px 32px 32px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 0px 16px 16px 16px;
  }
`
