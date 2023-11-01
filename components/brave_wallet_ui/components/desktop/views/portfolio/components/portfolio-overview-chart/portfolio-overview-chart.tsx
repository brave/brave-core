// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import { TokenPriceHistory } from '../../../../../../constants/types'

// components
import LineChart from '../../../../line-chart'

// style
import { Column } from '../../../../../shared/style'

interface Props {
  hasZeroBalance: boolean
  portfolioPriceHistory: TokenPriceHistory[] | undefined
  isLoading: boolean
}

export const PortfolioOverviewChart: React.FC<Props> = ({
  hasZeroBalance,
  portfolioPriceHistory,
  isLoading
}) => {
  // memos
  const priceHistory = React.useMemo((): TokenPriceHistory[] => {
    if (hasZeroBalance || !portfolioPriceHistory) {
      return []
    }
    return portfolioPriceHistory
  }, [portfolioPriceHistory, hasZeroBalance])

  // render
  return (
    <Column
      alignItems='center'
      fullWidth
    >
      <LineChart
        priceData={priceHistory}
        isLoading={hasZeroBalance ? false : isLoading}
        isDisabled={hasZeroBalance}
      />
    </Column>
  )
}

export default PortfolioOverviewChart
