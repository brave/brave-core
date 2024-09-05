// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

// Shared Styles
import { Row } from '../../shared/style'
import {
  layoutSmallWidth //
} from '../wallet-page-wrapper/wallet-page-wrapper.style'

export const HeaderWrapper = styled(Row)`
  padding: 24px 24px 0px 24px;
  @media screen and (max-width: ${layoutSmallWidth}px) {
    padding: 16px 16px 0px 16px;
  }
`
