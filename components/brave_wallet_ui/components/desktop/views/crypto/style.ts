// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

// Shared Styles
import { Row } from '../../../shared/style'
import {
  layoutSmallWidth //
} from '../../wallet-page-wrapper/wallet-page-wrapper.style'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  position: relative;
  height: 100%;
`

export const SegmentedControlsWrapperWeb3 = styled(Row)`
  padding: 24px 24px 14px 24px;
  @media screen and (max-width: ${layoutSmallWidth}px) {
    padding: 16px 16px 6px 16px;
  }
`

export const SegmentedControlsWrapperMarket = styled(Row)`
  padding: 4px 24px 24px 24px;
  @media screen and (max-width: ${layoutSmallWidth}px) {
    padding: 0px 0px 16px 0px;
  }
`
