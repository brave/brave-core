// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  LineChart as RechartsLineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  Legend,
  ResponsiveContainer,
} from 'recharts'

export interface ChartData {
  data: Array<Record<string, string | number>>
  labels?: Record<string, string>
}

const CHART_COLORS = [
  'var(--leo-color-blurple-40)',
  'var(--leo-color-green-40)',
  'var(--leo-color-red-40)',
  'var(--leo-color-yellow-40)',
  'var(--leo-color-purple-40)',
  'var(--leo-color-teal-40)',
  'var(--leo-color-orange-40)',
  'var(--leo-color-purple-40)',
  'var(--leo-color-blue-40)',
]

interface LineChartProps {
  chartData: ChartData
}

export default function LineChart({ chartData }: LineChartProps) {
  const dataKeys = Object.keys(chartData.data[0]).filter((key) => key !== 'x')

  return (
    <ResponsiveContainer
      width='100%'
      height='100%'
    >
      <RechartsLineChart
        data={chartData.data}
        margin={{ top: 5, right: 5, left: -5, bottom: 5 }}
      >
        <CartesianGrid strokeDasharray='3 3' />
        <XAxis dataKey='x' />
        <YAxis />
        <Tooltip />
        <Legend />
        {dataKeys.map((key, index) => (
          <Line
            key={key}
            type='monotone'
            dataKey={key}
            stroke={CHART_COLORS[index % CHART_COLORS.length]}
            name={chartData.labels?.[key] ?? key}
            strokeWidth={2}
          />
        ))}
      </RechartsLineChart>
    </ResponsiveContainer>
  )
}
