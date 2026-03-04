// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Meta } from '@storybook/react'
import * as mojom from '../../../common/mojom'
import { ExtractedDeepResearchEvents } from './deep_research_utils'
import DeepResearchEvent from './deep_research_event'

function DynamicProgressStory() {
  const [deepResearch, setDeepResearch] =
    React.useState<ExtractedDeepResearchEvents>({
      thinkingEvents: [],
      hasDeepResearchEvents: true,
      hasSynthesisCompletion: false,
    })

  React.useEffect(() => {
    const steps: Array<{
      delay: number
      update: Partial<ExtractedDeepResearchEvents>
    }> = [
      {
        delay: 500,
        update: {
          queriesEvent: {
            queries: [
              'quantum computing breakthroughs',
              'quantum error correction',
              'quantum supremacy experiments',
            ],
          },
        },
      },
      {
        delay: 2000,
        update: {
          searchStatusEvent: {
            status: mojom.DeepResearchSearchStatus.kStarted,
            query: 'quantum computing breakthroughs',
            queryIndex: 0,
            totalQueries: 3,
            urlsFound: 0,
            elapsedMs: 0,
          },
        },
      },
      {
        delay: 4000,
        update: {
          searchStatusEvent: {
            status: mojom.DeepResearchSearchStatus.kCompleted,
            query: 'quantum computing breakthroughs',
            queryIndex: 0,
            totalQueries: 3,
            urlsFound: 12,
            elapsedMs: 1800,
          },
        },
      },
      {
        delay: 5000,
        update: {
          fetchStatusEvent: {
            query: 'quantum computing breakthroughs',
            urlsTotal: 12,
            urlsFetched: 4,
          },
        },
      },
      {
        delay: 6500,
        update: {
          fetchStatusEvent: {
            query: 'quantum computing breakthroughs',
            urlsTotal: 12,
            urlsFetched: 12,
          },
          analysisStatusEvent: {
            status: mojom.DeepResearchAnalysisStatus.kStarted,
            query: 'quantum computing breakthroughs',
            chunksAnalyzed: 0,
            chunksTotal: 24,
          },
        },
      },
      {
        delay: 8000,
        update: {
          analysisStatusEvent: {
            status: mojom.DeepResearchAnalysisStatus.kProgress,
            query: 'quantum computing breakthroughs',
            chunksAnalyzed: 12,
            chunksTotal: 24,
          },
        },
      },
      {
        delay: 10000,
        update: {
          searchStatusEvent: {
            status: mojom.DeepResearchSearchStatus.kStarted,
            query: 'quantum error correction advances',
            queryIndex: 1,
            totalQueries: 3,
            urlsFound: 0,
            elapsedMs: 0,
          },
          progressEvent: {
            elapsedSeconds: 10,
            iterationCount: 1,
            queriesCount: 2,
            urlsAnalyzed: 12,
            snippetsAnalyzed: 24,
          },
        },
      },
      {
        delay: 13000,
        update: {
          searchStatusEvent: {
            status: mojom.DeepResearchSearchStatus.kCompleted,
            query: 'quantum error correction advances',
            queryIndex: 1,
            totalQueries: 3,
            urlsFound: 8,
            elapsedMs: 2100,
          },
        },
      },
      {
        delay: 16000,
        update: {
          completeEvent: { reason: 'All research iterations finished' },
          progressEvent: {
            elapsedSeconds: 16,
            iterationCount: 2,
            queriesCount: 3,
            urlsAnalyzed: 20,
            snippetsAnalyzed: 42,
          },
        },
      },
    ]

    const timeouts = steps.map(({ delay, update }) =>
      setTimeout(() => {
        setDeepResearch((prev) => ({ ...prev, ...update }))
      }, delay),
    )

    return () => timeouts.forEach(clearTimeout)
  }, [])

  return (
    <DeepResearchEvent
      deepResearch={deepResearch}
      isActive={true}
    />
  )
}

export const DynamicProgress = {
  render: () => <DynamicProgressStory />,
}

export const ActiveProgress = {
  render: () => (
    <DeepResearchEvent
      deepResearch={{
        searchStatusEvent: {
          status: mojom.DeepResearchSearchStatus.kStarted,
          query: 'quantum computing breakthroughs 2026',
          queryIndex: 2,
          totalQueries: 5,
          urlsFound: 0,
          elapsedMs: 0,
        },
        thinkingEvents: [
          {
            query: 'quantum error correction advances',
            chunksAnalyzed: 12,
            chunksSelected: 4,
            urlsAnalyzed: 8,
            urlsSelected: [
              { url: 'https://example.com/1' },
              { url: 'https://example.com/2' },
            ],
            urlsInfo: [],
          },
        ],
        progressEvent: {
          elapsedSeconds: 45,
          iterationCount: 2,
          queriesCount: 3,
          urlsAnalyzed: 14,
          snippetsAnalyzed: 28,
        },
        hasDeepResearchEvents: true,
        hasSynthesisCompletion: false,
      }}
      isActive={true}
    />
  ),
}

export const Error = {
  render: () => (
    <DeepResearchEvent
      deepResearch={{
        thinkingEvents: [],
        errorEvent: {
          error: 'Deep research service timed out after 120 seconds',
        },
        hasDeepResearchEvents: true,
        hasSynthesisCompletion: false,
      }}
      isActive={false}
    />
  ),
}

export const ResearchComplete = {
  render: () => (
    <DeepResearchEvent
      deepResearch={{
        thinkingEvents: [],
        completeEvent: {
          reason: 'All research iterations finished',
        },
        hasDeepResearchEvents: true,
        hasSynthesisCompletion: false,
      }}
      isActive={false}
    />
  ),
}

export default {
  title: 'Chat/DeepResearchEvent',
  component: DeepResearchEvent,
} as Meta<typeof DeepResearchEvent>
