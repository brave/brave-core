// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import LineChart, { ChartData } from './line_chart'

function parseChartData(search: string): ChartData | null {
  const encodedData = search.replace('?', '')
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

export function App({ searchQuery }: { searchQuery?: string }) {
  const chartData = parseChartData(searchQuery ?? window.location.search)

  if (!chartData?.data || chartData.data.length === 0) {
    return null
  }

  return <LineChart chartData={chartData} />
}

function initialize() {
  const root = createRoot(document.getElementById('root')!)
  root.render(<App />)
}

document.addEventListener('DOMContentLoaded', initialize)
