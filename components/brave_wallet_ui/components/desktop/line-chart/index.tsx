import * as React from 'react'
import { CSSProperties } from 'styled-components'

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
            <linearGradient id='lineGradient' x1='0' y1='0' x2='1' y2='0'>
              <stop offset='0%' stopColor='#F73A1C' stopOpacity={1} />
              <stop offset='50%' stopColor='#BF14A2' stopOpacity={1} />
              <stop offset='100%' stopColor='#6F4CD2' stopOpacity={1} />
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
            stroke={isAsset ? isDown ? '#EE6374' : '#2AC194' : priceData.length <= 0 ? '#BF14A2' : 'url(#lineGradient)'}
            fill='none'
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
