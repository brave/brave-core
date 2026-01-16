/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Meta, StoryObj } from '@storybook/react'

import { createMockShieldsAPI } from '../api/shields_api_mock'
import { ShieldsApiProvider } from '../api/shields_api_context'
import { App } from '../components/app'

interface StorybookAppProps {}

function StorybookApp(props: StorybookAppProps) {
  return (
    <div style={{ position: 'absolute', inset: 0 }}>
      <ShieldsApiProvider api={createMockShieldsAPI()}>
        <App />
      </ShieldsApiProvider>
    </div>
  )
}

export default {
  title: 'Shields Panel/Shields',
  component: StorybookApp,
} satisfies Meta<typeof StorybookApp>

export const ShieldsPanel: StoryObj<typeof StorybookApp> = {}
