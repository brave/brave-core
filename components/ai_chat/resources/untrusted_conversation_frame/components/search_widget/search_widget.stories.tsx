// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import type { Meta, StoryObj } from '@storybook/react'
import SearchWidget from './search_widget'
import { provideStrings } from '../../../../../../.storybook/locale'

provideStrings({
  apiHost: 'http://localhost:8000',
})

const meta: Meta<typeof SearchWidget> = {
  title: 'SearchWidget',
  component: SearchWidget,
}

export default meta
type Story = StoryObj<typeof SearchWidget>

export const Default: Story = {
  args: {
    query: 'brave',
    type: 'web',
  },
}
