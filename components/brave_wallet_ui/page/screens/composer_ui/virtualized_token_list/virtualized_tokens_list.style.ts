// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// Shared Styles
import { Row } from '../../../../components/shared/style'
import {
  layoutSmallWidth //
} from '../../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper.style'

export const ListItemWrapper = styled(Row)`
  padding: 0px 40px;
  @media screen and (max-width: ${layoutSmallWidth}px) {
    padding: 0px 8px;
  }
`
