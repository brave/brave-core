/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Meta, StoryObj } from '@storybook/react'

import { createShieldsStore } from '../lib/mock_shields_store'
import { ShieldsContext } from '../lib/shields_context'
import { App } from '../components/app'

function StorybookApp() {
  return (
    <div style={{ position: 'absolute', inset: 0 }}>
      <ShieldsContext.Provider value={createShieldsStore()}>
        <App />
      </ShieldsContext.Provider>
    </div>
  )
}

export default {
  title: 'Shields Panel/Shields',
  component: StorybookApp,
} satisfies Meta<typeof StorybookApp>

export const ShieldsPanel: StoryObj<typeof StorybookApp> = {}
