// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import {
  LineChartIframeData,
  TokenPriceHistory
} from '../../../../../../constants/types'

// utils
import {
  useSafeWalletSelector //
} from '../../../../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../../../../common/selectors'

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
  // redux
  const hidePortfolioBalances = useSafeWalletSelector(
    WalletSelectors.hidePortfolioBalances
  )
  const defaultFiatCurrency = useSafeWalletSelector(
    WalletSelectors.defaultFiatCurrency
  )

  // memos
  const encodedPriceData = React.useMemo(() => {
    const iframeData: LineChartIframeData = {
      defaultFiatCurrency,
      hidePortfolioBalances,
      priceData:
        hasZeroBalance || !portfolioPriceHistory ? [] : portfolioPriceHistory
    }
    return encodeURIComponent(JSON.stringify(iframeData))
  }, [
    portfolioPriceHistory,
    hasZeroBalance,
    defaultFiatCurrency,
    hidePortfolioBalances
  ])

  // render
  return (
    <Column
      alignItems='center'
      fullWidth
    >
      <iframe
        width={'100%'}
        height={'130px'}
        frameBorder={0}
        src={`chrome-untrusted://line-chart-display${
          isLoading ? '' : `?${encodedPriceData}`
        }`}
        sandbox='allow-scripts'
      />
    </Column>
  )
}

export default PortfolioOverviewChart
