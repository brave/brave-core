// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Meta } from '@storybook/react'
import * as mojom from '../../../common/mojom'
import DeepResearchEvent from './deep_research_event'

export const ActiveProgress = {
  render: () => (
    <DeepResearchEvent
      searchStatusEvent={{
        status: mojom.DeepResearchSearchStatus.kStarted,
        query: 'quantum computing breakthroughs 2026',
        queryIndex: 2,
        totalQueries: 5,
        urlsFound: 0,
        elapsedMs: 0,
      }}
      thinkingEvents={[
        {
          query: 'quantum error correction advances',
          chunksAnalyzed: 12,
          chunksSelected: 4,
          urlsAnalyzed: 8,
          urlsSelected: [{url: 'https://example.com/1'}, {url: 'https://example.com/2'}],
          urlsInfo: [],
        },
      ]}
      progressEvent={{
        elapsedSeconds: 45,
        iterationCount: 2,
        queriesCount: 3,
        urlsAnalyzed: 14,
        snippetsAnalyzed: 28,
      }}
      isActive={true}
    />
  ),
}

export const Error = {
  render: () => (
    <DeepResearchEvent
      thinkingEvents={[]}
      errorEvent={{
        error: 'Deep research service timed out after 120 seconds',
      }}
      isActive={false}
    />
  ),
}

export const ResearchComplete = {
  render: () => (
    <DeepResearchEvent
      thinkingEvents={[]}
      completeEvent={{
        reason: 'All research iterations finished',
      }}
      isActive={false}
    />
  ),
}

export default {
  title: 'Chat/DeepResearchEvent',
  component: DeepResearchEvent,
} as Meta<typeof DeepResearchEvent>
