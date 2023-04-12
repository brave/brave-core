// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { CSSProperties } from 'styled-components'
import * as leo from '@brave/leo/tokens/css'

import {
  AreaChart,
  Area,
  XAxis,
  YAxis,
  ResponsiveContainer,
  ReferenceDot,
  Tooltip
} from 'recharts'

import { PriceDataObjectType } from '../../../constants/types'
import CustomTooltip from './custom-tooltip'

// Styled Components
import {
  StyledWrapper,
  LoadingOverlay,
  LoadIcon
} from './style'
import { CustomReferenceDot } from './custom-reference-dot'

// #EE6374 and #2AC194 do not exist in design system,
// will be updated in future design work.
export const assetDownColor = '#EE6374'
export const assetUpColor = '#2AC194'

export interface Props {
  priceData: PriceDataObjectType[]
  onUpdateBalance: (value: number | undefined) => void
  isAsset: boolean
  isDown: boolean
  isLoading: boolean
  isDisabled: boolean
  showPulsatingDot?: boolean
  customStyle?: CSSProperties
  showTooltip?: boolean
}

const EmptyChartData = [
  {
    date: '1',
    close: 1
  },
  {
    date: '2',
    close: 1
  },
  {
    date: '3',
    close: 1
  }
]

function LineChart ({
  priceData,
  onUpdateBalance,
  isAsset,
  isDown,
  isLoading,
  isDisabled,
  customStyle,
  showPulsatingDot,
  showTooltip
}: Props) {
  // state
  const [position, setPosition] = React.useState<number>(0)

  // memos / computed
  const chartData = React.useMemo(() => {
    if (priceData.length <= 0 || isDisabled) {
      return EmptyChartData
    }
    return priceData
  }, [priceData, isDisabled])

  const lastPoint = chartData.length - 1

  // methods
  const onChartMouseLeave = React.useCallback(() => onUpdateBalance(undefined), [onUpdateBalance])
  const onUpdatePosition = React.useCallback((value: number) => setPosition(value), [])

  // render
  return (
    <StyledWrapper style={customStyle}>
      <LoadingOverlay isLoading={isLoading}>
        <LoadIcon />
      </LoadingOverlay>
      <ResponsiveContainer width='99%' height='99%'>
        <AreaChart
          data={chartData}
          margin={{ top: 5, left: 8, right: 8, bottom: 0 }}
          onMouseLeave={onChartMouseLeave}
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
          <YAxis hide={true} domain={['auto', 'auto']} />
          <XAxis hide={true} dataKey='date' />
          {priceData.length > 0 && !isDisabled && showTooltip &&
            <Tooltip
              isAnimationActive={false}
              position={{ x: position, y: 0 }}
              content={
                <CustomTooltip
                  onUpdateBalance={onUpdateBalance}
                  onUpdatePosition={onUpdatePosition}
                />
              }
            />
          }
          <Area
            isAnimationActive={false}
            type='monotone'
            dataKey='close'
            strokeWidth={2}
            stroke={
              isAsset
                ? isDown
                  ? assetDownColor
                  : assetUpColor
                : leo.color.icon.interactive}
            fill={
              isAsset ||
                priceData.length <= 0
                ? 'none'
                : 'url(#portfolioGradient)'}
            activeDot={
              {
                stroke:
                  isAsset
                    ? leo.color.white
                    : leo.color.icon.interactive,
                fill:
                  isAsset
                    ? isDown
                      ? assetDownColor
                      : assetUpColor
                    : leo.color.white,
                strokeWidth: 2,
                r: isAsset ? 4 : 5
              }}
          />
          {showPulsatingDot &&
            <ReferenceDot
              x={chartData[lastPoint].date.toString()}
              y={chartData[lastPoint].close}
              shape={
                <CustomReferenceDot
                  cx={chartData[lastPoint].date.toString()}
                  cy={chartData[lastPoint].close}
                  isAsset={isAsset}
                  isDown={isDown}
                />
              }
            />
          }
        </AreaChart>
      </ResponsiveContainer>
    </StyledWrapper>
  )
}

LineChart.defaultProps = {
  showPulsatingDot: true,
  showTooltip: true
}

export default LineChart
