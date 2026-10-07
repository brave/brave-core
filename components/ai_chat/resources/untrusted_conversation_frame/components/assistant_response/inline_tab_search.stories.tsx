// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Meta } from '@storybook/react'
import { InferControlsFromArgs } from '$storybook/utils'
import * as Mojom from '../../../common/mojom'
import MockContext from '../../mock_untrusted_conversation_context'
import InlineTabSearch from './inline_tab_search'

type CustomArgs = {
  tabCount: number
  hasUntitledTab: boolean
  isSearching: boolean
  isUnsupported: boolean
}

const args: CustomArgs = {
  tabCount: 3,
  hasUntitledTab: false,
  isSearching: false,
  isUnsupported: false,
}

const sampleTabs = [
  {
    title: 'React Hooks – Reference',
    url: 'https://react.dev/reference/react',
  },
  {
    title: 'Understanding useEffect',
    url: 'https://overreacted.io/a-complete-guide-to-useeffect/',
  },
  {
    title:
      'A very long tab title that should be truncated with an ellipsis '
      + 'once it runs out of room in the card',
    url: 'https://developer.mozilla.org/en-US/docs/Web/JavaScript',
  },
  { title: 'Brave Search', url: 'https://search.brave.com/' },
  { title: 'GitHub – brave/brave-core', url: 'https://github.com/brave' },
]

export const _InlineTabSearch = {
  render: (args: CustomArgs) => {
    const tabs: Mojom.TabData[] = sampleTabs
      .slice(0, args.tabCount)
      .map((tab, index) => ({
        id: index + 1,
        contentId: (index + 1) * 10,
        title: args.hasUntitledTab && index === 0 ? '' : tab.title,
        url: { url: tab.url },
      }))

    const searchForTabs = async () => {
      if (args.isSearching) {
        // Never resolves, so the story stays in the pending state.
        return new Promise<{ tabs: Mojom.TabData[] | null }>(() => {})
      }
      return { tabs: args.isUnsupported ? null : tabs }
    }

    return (
      <MockContext uiHandler={{ searchForTabs }}>
        <InlineTabSearch query='react hooks' />
      </MockContext>
    )
  },
}

export default {
  title: 'AI Chat/InlineTabSearch',
  component: InlineTabSearch,
  argTypes: InferControlsFromArgs(args),
  args,
} as Meta<typeof InlineTabSearch>
