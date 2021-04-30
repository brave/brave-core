import * as React from 'react'
import {
  AreaChart,
  Area,
  XAxis,
  YAxis,
  ResponsiveContainer,
  ReferenceDot
} from 'recharts'

import { PriceDataObjectType } from '../../../constants/types'

// Styled Components
import {
  StyledWrapper
} from './style'

export interface Props {
  priceData: PriceDataObjectType[]
}

// This is an animated Pulsating dot that will render at the end
// of the line chart for the current price.
const CustomReferenceDot = (props: any) => {
  return (
    <>
      <circle fill='none' cx={props.cx} r='3' cy={props.cy} stroke='#51CF66' strokeWidth='1'>
        <animate attributeName='r' values='3;8;3;3' dur='3s' begin='0s' repeatCount='indefinite' />
        <animate attributeName='opacity' values='1;0;0;0' dur='3s' begin='0s' repeatCount='indefinite' />
      </circle >
      <circle fill='#51CF66' cx={props.cx} r='3' cy={props.cy} />
    </>
  )
}

export default class LineChart extends React.PureComponent<Props, {}> {
  render () {
    const { priceData } = this.props
    const lastPoint = priceData.length - 1
    return (
      <StyledWrapper>
        <ResponsiveContainer width='99%' height='99%'>
          <AreaChart
            data={priceData}
            margin={{ top: 5, left: 8, right: 8, bottom: 0 }}
          >
            <YAxis hide={true} domain={['auto', 'auto']} />
            <XAxis hide={true} dataKey='date' />
            <Area
              isAnimationActive={true}
              type='monotone'
              dataKey='close'
              strokeWidth={2}
              stroke='#51CF66'
              fill='none'
            />
            <ReferenceDot x={priceData[lastPoint].date} y={priceData[lastPoint].close} shape={CustomReferenceDot} />
          </AreaChart>
        </ResponsiveContainer>
      </StyledWrapper>
    )
  }
}
