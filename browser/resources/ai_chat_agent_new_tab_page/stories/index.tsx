// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import type { Meta, StoryObj } from '@storybook/react'
import '../strings'

// Components
import { App } from '../components/app'

export default {
  title: 'New Tab Page/Content Agent New Tab Page',
  component: App,
  parameters: {
    layout: 'fullscreen',
  },
} satisfies Meta<typeof App>

export const Default: StoryObj<typeof App> = {}
