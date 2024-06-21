// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types & constants
import {
  BraveWallet,
  LineChartIframeData,
  TokenPriceHistory
} from '../../../../../../constants/types'
import {
  LOCAL_STORAGE_KEYS //
} from '../../../../../../common/constants/local-storage-keys'

// utils
import {
  useGetDefaultFiatCurrencyQuery //
} from '../../../../../../common/slices/api.slice'
import {
  useSyncedLocalStorage //
} from '../../../../../../common/hooks/use_local_storage'

// components
import {
  LineChartControls //
} from '../../../../line-chart/line-chart-controls/line-chart-controls'

// style
import { Column } from '../../../../../shared/style'
import { SelectTimelineWrapper } from '../../style'

interface Props {
  hasZeroBalance: boolean
  portfolioPriceHistory: TokenPriceHistory[] | undefined
  isLoading: boolean
  timeframe: BraveWallet.AssetPriceTimeframe
  onTimeframeChanged: (timeframe: BraveWallet.AssetPriceTimeframe) => void
}

export const PortfolioOverviewChart: React.FC<Props> = ({
  hasZeroBalance,
  portfolioPriceHistory,
  isLoading,
  timeframe,
  onTimeframeChanged
}) => {
  // local-Storage
  const [hidePortfolioBalances] = useSyncedLocalStorage(
    LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_BALANCES,
    false
  )

  // state
  const [isIframeLoaded, setIsIframeLoaded] = React.useState<boolean>(false)

  // queries
  const { data: defaultFiatCurrency = 'usd' } = useGetDefaultFiatCurrencyQuery()

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

  // methods
  const handleOnLoad = () => {
    setIsIframeLoaded(true)
  }

  // render
  return (
    <>
      <SelectTimelineWrapper
        padding='0px 32px'
        marginBottom={8}
      >
        <LineChartControls
          onSelectTimeline={onTimeframeChanged}
          selectedTimeline={timeframe}
        />
      </SelectTimelineWrapper>
      <Column
        alignItems='center'
        fullWidth
      >
        <iframe
          onLoad={handleOnLoad}
          width={'100%'}
          height={'130px'}
          frameBorder={0}
          src={`chrome-untrusted://line-chart-display${
            isLoading || !isIframeLoaded ? '' : `?${encodedPriceData}`
          }`}
          sandbox='allow-scripts allow-same-origin'
        />
      </Column>
    </>
  )
}

export default PortfolioOverviewChart
