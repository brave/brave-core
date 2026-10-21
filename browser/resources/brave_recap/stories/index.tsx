// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Meta, StoryObj } from '@storybook/react'
import { App } from '../components/app'
import '../components/app.css'

const DAY_MS = 24 * 60 * 60 * 1000

function RecapStory(props: { adsBlocked: number }) {
  return (
    <div style={{ width: '100%', height: '100vh' }}>
      <App
        stats={{
          adsBlocked: props.adsBlocked,
          bandwidthSavedBytes: 25_812_345_678,
          firstRunTime: Date.now() - 731 * DAY_MS,
        }}
      />
    </div>
  )
}

export default {
  title: 'Brave Recap/Page',
  component: RecapStory,
  args: { adsBlocked: 1_284_213 },
} satisfies Meta<typeof RecapStory>

export const Default: StoryObj<typeof RecapStory> = {}

export const NotEnoughData: StoryObj<typeof RecapStory> = {
  args: { adsBlocked: 12 },
}
