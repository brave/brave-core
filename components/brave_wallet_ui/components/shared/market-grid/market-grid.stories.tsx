// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { Story, Meta } from '@storybook/react'
import { GridHeader, MarketGrid, MarketGridProps } from './market-grid'

export default {
  title: 'Wallet/Desktop/Components/MarketGrid',
  component: MarketGrid
} as Meta

const Template: Story<MarketGridProps> = (args) => <MarketGrid {...args} />

export const Default = Template.bind({})
Default.args = {
  headers: [
    { label: 'ID', },
    { label: 'Name', width: '200px' },
    { label: 'Email' }
  ] as GridHeader[],
  rows: [
    ['1', 'John Doe', 'john@example.com'],
    ['2', 'Jane Smith', 'jane@example.com'],
    ['3', 'Bob Johnson', 'bob@example.com'],
    ['4', 'Alice Williams', 'alice@example.com'],
    ['5', 'Mike Brown', 'mike@example.com']
  ],
  visibleRows: 5,
  rowHeight: 50
}

export const WithOnClick = Template.bind({})
WithOnClick.args = {
  headers: [
    { label: 'ID' },
    { label: 'Name' },
    { label: 'Email' }
  ] as GridHeader[],
  rows: [
    ['1', 'John Doe', 'john@example.com'],
    ['2', 'Jane Smith', 'jane@example.com'],
    ['3', 'Bob Johnson', 'bob@example.com'],
    ['4', 'Alice Williams', 'alice@example.com'],
    ['5', 'Mike Brown', 'mike@example.com']
  ],
  visibleRows: 5,
  rowHeight: 50,
  onRowClick: (rowIndex: number) => {
    console.log(`Clicked row ${rowIndex}`)
  }
}

export const WithSorting = Template.bind({})
WithSorting.args = {
  headers: [
    { label: 'ID', width: '100px', sortable: true },
    { label: 'Name', width: '200px', sortable: true },
    { label: 'Email', sortable: true }
  ] as GridHeader[],
  rows: [
    ['1', 'John Doe', 'john@example.com'],
    ['2', 'Jane Smith', 'jane@example.com'],
    ['3', 'Bob Johnson', 'bob@example.com'],
    ['4', 'Alice Williams', 'alice@example.com'],
    ['5', 'Mike Brown', 'mike@example.com']
  ],
  visibleRows: 5,
  rowHeight: 50
}

export const HideOnSmallScreen = Template.bind({})
HideOnSmallScreen.args = {
  headers: [
    { label: 'ID', width: '50px' },
    { label: 'Name' },
    { label: 'Email', hideOnSmall: true, hideOnPanel: true}
  ] as GridHeader[],
  rows: [
    ['1', 'John Doe', 'john@example.com'],
    ['2', 'Jane Smith', 'jane@example.com'],
    ['3', 'Bob Johnson', 'bob@example.com'],
    ['4', 'Alice Williams', 'alice@example.com'],
    ['5', 'Mike Brown', 'mike@example.com']
  ],
  visibleRows: 5,
  rowHeight: 50
}
