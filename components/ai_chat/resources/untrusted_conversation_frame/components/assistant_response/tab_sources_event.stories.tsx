// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Meta } from '@storybook/react'
import { InferControlsFromArgs } from '../../../../../../.storybook/utils'
import * as Mojom from '../../../common/mojom'
import MockContext from '../../mock_untrusted_conversation_context'
import TabSourcesEvent from './tab_sources_event'

type CustomArgs = {
  tabCount: number
  hasUntitledTab: boolean
}

const args: CustomArgs = {
  tabCount: 3,
  hasUntitledTab: false,
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

export const _TabSourcesEvent = {
  render: (args: CustomArgs) => {
    const sources = sampleTabs.slice(0, args.tabCount).map((tab, index) => ({
      tab_id: index + 1,
      title: args.hasUntitledTab && index === 0 ? '' : tab.title,
      url: tab.url,
    }))

    const artifacts: Mojom.ToolArtifact[] = [
      {
        id: null,
        type: Mojom.TAB_SOURCES_ARTIFACT_TYPE,
        contentJson: JSON.stringify({ sources }),
      },
    ]

    return (
      <MockContext>
        <TabSourcesEvent artifacts={artifacts} />
      </MockContext>
    )
  },
}

export default {
  title: 'AI Chat/TabSourcesEvent',
  component: TabSourcesEvent,
  argTypes: InferControlsFromArgs(args),
  args,
} as Meta<typeof TabSourcesEvent>
