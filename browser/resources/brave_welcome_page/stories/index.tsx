/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Meta, StoryObj } from '@storybook/react'

import { App } from '../components/app'

interface StorybookAppProps {}

function StorybookApp(props: StorybookAppProps) {
  return (
    <div style={{ position: 'absolute', inset: 0 }}>
      <App />
    </div>
  )
}

export default {
  title: 'Welcome Page/App',
  component: StorybookApp,
} satisfies Meta<typeof StorybookApp>

export const NewTabPage: StoryObj<typeof StorybookApp> = {}
