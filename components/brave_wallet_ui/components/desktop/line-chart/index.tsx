import * as React from 'react'
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
import theme from 'brave-ui/theme/colors/'
import CustomTooltip from './custom-tooltip'

// Styled Components
import {
  StyledWrapper,
  LoadingOverlay,
  LoadIcon
} from './style'

export interface Props {
  priceData: PriceDataObjectType[]
  onUpdateBalance: (value: number | undefined) => void
  isAsset: boolean
  isDown: boolean
  isLoading: boolean
  isDisabled: boolean
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

function LineChart (props: Props) {
  const { priceData, onUpdateBalance, isAsset, isDown, isLoading, isDisabled } = props
  const [position, setPosition] = React.useState<number>(0)

  const chartData = React.useMemo(() => {
    if (priceData.length <= 0 || isDisabled) {
      return EmptyChartData
    }
    return priceData
  }, [priceData, isDisabled])

  const lastPoint = chartData.length - 1

  // This is an animated Pulsating dot that will render at the end
  // of the line chart for the current price.
  const CustomReferenceDot = (props: any) => {
    return (
      <>
        <circle fill='none' cx={props.cx} r='3' cy={props.cy} stroke={isAsset ? isDown ? theme.red600 : theme.teal600 : '#BF14A2'} strokeWidth='1'>
          <animate attributeName='r' values='3;8;3;3' dur='3s' begin='0s' repeatCount='indefinite' />
          <animate attributeName='opacity' values='1;0;0;0' dur='3s' begin='0s' repeatCount='indefinite' />
        </circle >
        <circle fill={isAsset ? isDown ? '#EE6374' : '#2AC194' : '#BF14A2'} cx={props.cx} r='3' cy={props.cy} />
      </>
    )
  }

  const onChartMouseLeave = () => {
    onUpdateBalance(undefined)
  }

  const onUpdatePosition = (value: number) => {
    setPosition(value)
  }

  return (
    <StyledWrapper>
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
          {priceData.length > 0 && !isDisabled &&
            <Tooltip
              isAnimationActive={false}
              position={{ x: position, y: 0 }}
              content={<CustomTooltip onUpdateBalance={onUpdateBalance} onUpdatePosition={onUpdatePosition} />}
            />
          }
          <Area
            isAnimationActive={false}
            type='monotone'
            dataKey='close'
            strokeWidth={2}
            stroke={isAsset ? isDown ? '#EE6374' : '#2AC194' : priceData.length <= 0 ? '#BF14A2' : `url(#lineGradient)`}
            fill='none'
          />
          <ReferenceDot x={chartData[lastPoint].date.toString()} y={chartData[lastPoint].close} shape={CustomReferenceDot} />
        </AreaChart>
      </ResponsiveContainer>
    </StyledWrapper>
  )
}

export default LineChart
