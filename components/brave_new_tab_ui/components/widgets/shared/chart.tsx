import * as React from 'react'
import { SVG } from './styles'

interface ChartConfig {
  width: number
  height: number
  data: Array<any>
}

interface ChartDataPoint {
  c: number
  h: number
  l: number
  o: number
  t: number
  v: number
}

const plotData = ({ data, height, width }: ChartConfig) => {
  const pointsPerDay = 4
  const daysInrange = 7
  const yHighs = data.map((point: ChartDataPoint) => point.h)
  const yLows = data.map((point: ChartDataPoint) => point.l)
  const dataPoints = data.map((point: ChartDataPoint) => point.c)
  const chartAreaY = height - 2
  const max = Math.max(...yHighs)
  const min = Math.min(...yLows)
  const pixelsPerPoint = (max - min) / chartAreaY
  return dataPoints
    .map((v, i) => {
      const y = (v - min) / pixelsPerPoint
      const x = i * (width / (pointsPerDay * daysInrange))
      return `${x},${chartAreaY - y}`
    })
    .join('\n')
}

export const Chart = ({ width, height, data }: ChartConfig) => {
  return (
    <SVG viewBox={`0 0 ${width} ${height}`}>
      <polyline
        fill='none'
        stroke='#44B0FF'
        strokeWidth='3'
        points={plotData({
          data,
          height,
          width
        })}
      />
    </SVG>
  )
}