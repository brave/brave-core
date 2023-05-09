// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../common/locale'

// Styled Components
import {
  HeaderTitle
} from './card-headers.style'
import { Row } from '../../shared/style'

export const PortfolioOverviewHeader = () => {
  return (
    <Row
      padding='24px 0px'
      justifyContent='space-between'
    >
      <HeaderTitle>
        {getLocale('braveWalletTopNavPortfolio')}
      </HeaderTitle>
    </Row>
  )
}

export default PortfolioOverviewHeader
