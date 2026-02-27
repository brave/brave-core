// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Meta } from '@storybook/react'
import { App } from './index'
import { ChartData } from './line_chart'

type CustomArgs = {
  chartData: ChartData
}

const args: CustomArgs = {
  chartData: {
    data: [
      { x: 'Jan', revenue: 4000, profit: 2400 },
      { x: 'Feb', revenue: 3000, profit: 1398 },
      { x: 'Mar', revenue: 2000, profit: 9800 },
      { x: 'Apr', revenue: 2780, profit: 3908 },
      { x: 'May', revenue: 1890, profit: 4800 },
      { x: 'Jun', revenue: 2390, profit: 3800 },
    ],
    labels: {
      revenue: 'Revenue',
      profit: 'Profit',
    },
  },
}

function makeSearchQuery(chartData: ChartData): string {
  return '?' + encodeURIComponent(JSON.stringify(chartData))
}

export const _ChartDisplay = {
  render: (args: CustomArgs) => {
    return (
      <div style={{ height: '400px' }}>
        <App searchQuery={makeSearchQuery(args.chartData)} />
      </div>
    )
  },
}

export default {
  title: 'Chat/ChartDisplay',
  component: App,
  argTypes: {
    chartData: { control: 'object' },
  },
  args,
} as Meta<typeof App>
