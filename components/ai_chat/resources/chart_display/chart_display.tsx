// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  Legend,
  ResponsiveContainer,
} from 'recharts'

interface ChartData {
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

function parseChartData(): ChartData | null {
  const encodedData = window.location.search.replace('?', '')
  if (!encodedData) {
    return null
  }
  try {
    const jsonString = decodeURIComponent(encodedData)
    return JSON.parse(jsonString) as ChartData
  } catch (e) {
    console.error('Failed to parse chart data:', e)
    return null
  }
}

function App() {
  const chartData = parseChartData()

  if (!chartData?.data || chartData.data.length === 0) {
    return null
  }

  const dataKeys = Object.keys(chartData.data[0]).filter((key) => key !== 'x')

  return (
    <ResponsiveContainer
      width='100%'
      height='100%'
    >
      <LineChart
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
      </LineChart>
    </ResponsiveContainer>
  )
}

function initialize() {
  const root = createRoot(document.getElementById('root')!)
  root.render(<App />)
}

document.addEventListener('DOMContentLoaded', initialize)
