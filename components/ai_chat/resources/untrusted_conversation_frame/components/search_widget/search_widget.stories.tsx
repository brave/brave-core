// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import type { Meta, StoryObj } from '@storybook/react'
import SearchWidget from './search_widget'

const meta: Meta<typeof SearchWidget> = {
  title: 'Chat/SearchWidget',
  component: SearchWidget,
}

export default meta
type Story = StoryObj<typeof SearchWidget>

export const Default: Story = {
  args: {
    query: 'Approach shoes',
    type: 'web',
    fetchResults: (type) => {
      let promise: Promise<any>
      switch (type) {
        case 'web':
          promise = import('./storybook-data/web.json', {
            assert: { type: 'json' },
          })
          break
        case 'images':
          promise = import('./storybook-data/images.json', {
            assert: { type: 'json' },
          })
          break
        case 'videos':
          promise = import('./storybook-data/videos.json', {
            assert: { type: 'json' },
          })
          break
        case 'news':
          promise = import('./storybook-data/news.json', {
            assert: { type: 'json' },
          })
          break
        default:
          throw new Error(`Unknown type: ${type}`)
      }

      return promise.then((r: any) => r.default)
    },
  },
}
