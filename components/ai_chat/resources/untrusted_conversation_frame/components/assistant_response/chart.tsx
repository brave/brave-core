// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
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
import * as Mojom from '../../../common/mojom'
import styles from './chart.module.scss'

interface ChartProps {
  artifact: Mojom.ToolArtifactContentBlock
}

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

export default function Chart({ artifact }: ChartProps) {
  const chartData = React.useMemo<ChartData | null>(() => {
    if (artifact.type !== Mojom.CHART_ARTIFACT_TYPE) {
      return null
    }

    try {
      return JSON.parse(artifact.contentJson) as ChartData
    } catch (e) {
      console.error('Failed to parse chart data:', e)
      return null
    }
  }, [artifact])

  if (!chartData?.data || chartData.data.length === 0) {
    return null
  }

  const dataKeys = React.useMemo(
    () => Object.keys(chartData.data[0]).filter((key) => key !== 'x'),
    [chartData],
  )

  return (
    <div className={styles.chartContainer}>
      <ResponsiveContainer
        width='100%'
        height='100%'
      >
        <LineChart
          data={chartData.data}
          margin={{ top: 5, right: 30, left: 20, bottom: 5 }}
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
    </div>
  )
}
