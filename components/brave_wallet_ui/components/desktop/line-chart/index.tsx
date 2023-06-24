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
  Tooltip
} from 'recharts'

import { PriceDataObjectType } from '../../../constants/types'
import CustomTooltip from './custom-tooltip'

// Styled Components
import {
  StyledWrapper,
  LoadingOverlay,
  LoadIcon,
  AreaWrapper
} from './style'
import { CustomReferenceDot } from './custom-reference-dot'

export interface Props {
  priceData: PriceDataObjectType[]
  isLoading: boolean
  isDisabled: boolean
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
  isLoading,
  isDisabled,
  customStyle,
  showTooltip
}: Props) {
  // state
  const [activeXPosition, setActiveXPosition] = React.useState<number>(0)
  const [activeYPosition, setActiveYPosition] = React.useState<number>(0)
  const [viewBoxHeight, setViewBoxHeight] = React.useState<number>(0)

  // memos / computed
  const chartData = React.useMemo(() => {
    if (priceData.length <= 0 || isDisabled) {
      return EmptyChartData
    }
    return priceData
  }, [priceData, isDisabled])

  const viewBoxHeightHalf = viewBoxHeight / 2
  const toolTipYPosition =
    activeYPosition < viewBoxHeightHalf
      ? activeYPosition
      : activeYPosition - 58

  // render
  return (
    <StyledWrapper style={customStyle}>
      <AreaWrapper>
      <ResponsiveContainer width='100%' height='99%'>
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
          <YAxis hide={true} domain={['auto', 'auto']} />
          <XAxis hide={true} dataKey='date' />
          {priceData.length > 0 && !isDisabled && showTooltip &&
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
                />
              }
            />
          }
          <Area
            isAnimationActive={false}
            type='monotone'
            dataKey='close'
            strokeWidth={2}
            stroke={leo.color.icon.interactive}
            fill={
              priceData.length <= 0
                ? 'none'
                : 'url(#portfolioGradient)'}
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

export default LineChart
