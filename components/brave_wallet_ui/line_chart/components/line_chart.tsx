// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { CSSProperties } from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import {
  AreaChart,
  Area,
  XAxis,
  YAxis,
  ResponsiveContainer,
  Tooltip
} from 'recharts'

// Types
import { TokenPriceHistory } from '../../constants/types'

// Utils
import { mojoTimeDeltaToJSDate } from '../../../common/mojomUtils'
import {
  deserializeTimeDelta,
  makeSerializableTimeDelta
} from '../../utils/model-serialization-utils'

// Components
import { CustomTooltip } from './custom_tooltip'
import { CustomReferenceDot } from './custom_reference_dot'

// Styled Components
import {
  StyledWrapper,
  LoadingOverlay,
  LoadIcon,
  AreaWrapper
} from './line_chart.styles'

interface Props {
  priceData: TokenPriceHistory[] | undefined
  isLoading: boolean
  isDisabled: boolean
  customStyle?: CSSProperties
  showTooltip?: boolean
  defaultFiatCurrency: string
  hidePortfolioBalances: boolean
}

const EmptyChartData = [
  {
    date: makeSerializableTimeDelta({
      microseconds: 1
    }),
    close: 1
  },
  {
    date: makeSerializableTimeDelta({
      microseconds: 2
    }),
    close: 1
  },
  {
    date: makeSerializableTimeDelta({
      microseconds: 3
    }),
    close: 1
  }
]

export function LineChart({
  priceData,
  isLoading,
  isDisabled,
  customStyle,
  showTooltip,
  defaultFiatCurrency,
  hidePortfolioBalances
}: Props) {
  // state
  const [activeXPosition, setActiveXPosition] = React.useState<number>(0)
  const [activeYPosition, setActiveYPosition] = React.useState<number>(0)
  const [viewBoxHeight, setViewBoxHeight] = React.useState<number>(0)

  // memos / computed
  const chartData = React.useMemo(() => {
    const priceHistory =
      !priceData || priceData.length <= 0 || isDisabled
        ? EmptyChartData
        : priceData

    return priceHistory.map((price) => ({
      date: mojoTimeDeltaToJSDate(deserializeTimeDelta(price.date)),
      close: price.close
    }))
  }, [priceData, isDisabled])

  const viewBoxHeightHalf = viewBoxHeight / 2
  const toolTipYPosition =
    activeYPosition < viewBoxHeightHalf ? activeYPosition : activeYPosition - 58

  // render
  return (
    <StyledWrapper style={customStyle}>
      <AreaWrapper>
        <ResponsiveContainer
          width='100%'
          height='99%'
        >
          <AreaChart
            data={chartData}
            margin={{ top: 5, left: 0, right: 0, bottom: 0 }}
          >
            <defs>
              <linearGradient
                id='portfolioGradient'
                x1='0'
                y1='0'
                x2='0'
                y2='1'
              >
                <stop
                  offset='0%'
                  stopColor={leo.color.icon.interactive}
                  stopOpacity={0.2}
                />
                <stop
                  offset='110%'
                  stopColor={leo.color.icon.interactive}
                  stopOpacity={0}
                />
              </linearGradient>
            </defs>
            <YAxis
              hide={true}
              domain={['auto', 'auto']}
            />
            <XAxis
              hide={true}
              dataKey='date'
            />
            {priceData &&
              priceData.length > 0 &&
              !isDisabled &&
              showTooltip && (
                <Tooltip
                  isAnimationActive={false}
                  position={{
                    x: activeXPosition,
                    y: toolTipYPosition
                  }}
                  cursor={{
                    stroke: leo.color.icon.interactive,
                    strokeWidth: 2
                  }}
                  content={
                    <CustomTooltip
                      onUpdateViewBoxHeight={setViewBoxHeight}
                      defaultFiatCurrency={defaultFiatCurrency}
                      hidePortfolioBalances={hidePortfolioBalances}
                    />
                  }
                />
              )}
            <Area
              isAnimationActive={false}
              type='monotone'
              dataKey='close'
              strokeWidth={2}
              stroke={leo.color.icon.interactive}
              fill={
                !priceData || priceData.length <= 0
                  ? 'none'
                  : 'url(#portfolioGradient)'
              }
              activeDot={
                <CustomReferenceDot
                  onUpdateYPosition={setActiveYPosition}
                  onUpdateXPosition={setActiveXPosition}
                />
              }
            />
          </AreaChart>
        </ResponsiveContainer>
      </AreaWrapper>
      <LoadingOverlay isLoading={isLoading}>
        <LoadIcon />
      </LoadingOverlay>
    </StyledWrapper>
  )
}

LineChart.defaultProps = {
  showPulsatingDot: true,
  showTooltip: true
}
