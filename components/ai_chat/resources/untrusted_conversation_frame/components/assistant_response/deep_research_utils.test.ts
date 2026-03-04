// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../../../common/mojom'
import {
  getEventTemplate,
  getCompletionEvent,
  getToolUseEvent,
} from '../../../common/test_data_utils'
import { extractDeepResearchEvents } from './deep_research_utils'

// Define enum values locally to avoid dependency on generated mojom build
// output, which may be stale or missing on CI/cross-platform environments.
// Values must match common.mojom enum ordering.
const DeepResearchSearchStatus = { kStarted: 0, kCompleted: 1 } as const
const DeepResearchAnalysisStatus = { kStarted: 0, kProgress: 1 } as const

function getDeepResearchToolUseEvent(): Mojom.ConversationEntryEvent {
  return getToolUseEvent({
    toolName: Mojom.DEEP_RESEARCH_TOOL_NAME,
    id: 'dr-1',
    argumentsJson: '{}',
    output: undefined,
  })
}

function getDeepResearchEvent(
  drEvent: Partial<Mojom.DeepResearchEvent>,
): Mojom.ConversationEntryEvent {
  return {
    ...getEventTemplate(),
    deepResearchEvent: drEvent as Mojom.DeepResearchEvent,
  }
}

describe('extractDeepResearchEvents', () => {
  it('should return empty results for undefined events', () => {
    const result = extractDeepResearchEvents(undefined)

    expect(result.hasDeepResearchEvents).toBe(false)
    expect(result.thinkingEvents).toHaveLength(0)
    expect(result.queriesEvent).toBeUndefined()
    expect(result.progressEvent).toBeUndefined()
    expect(result.completeEvent).toBeUndefined()
    expect(result.errorEvent).toBeUndefined()
    expect(result.hasSynthesisCompletion).toBe(false)
  })

  it('should return empty results for events with no deep research', () => {
    const events = [getCompletionEvent('Hello')]
    const result = extractDeepResearchEvents(events)

    expect(result.hasDeepResearchEvents).toBe(false)
    expect(result.thinkingEvents).toHaveLength(0)
  })

  it('should detect hasDeepResearchEvents from ToolUseEvent', () => {
    const events = [getDeepResearchToolUseEvent()]
    const result = extractDeepResearchEvents(events)

    expect(result.hasDeepResearchEvents).toBe(true)
  })

  it('should detect hasDeepResearchEvents from deep research events', () => {
    const events = [
      getDeepResearchEvent({
        queriesEvent: { queries: ['test query'] },
      }),
    ]
    const result = extractDeepResearchEvents(events)

    expect(result.hasDeepResearchEvents).toBe(true)
  })

  it('should extract queriesEvent', () => {
    const events = [
      getDeepResearchToolUseEvent(),
      getDeepResearchEvent({
        queriesEvent: { queries: ['query 1', 'query 2'] },
      }),
    ]
    const result = extractDeepResearchEvents(events)

    expect(result.queriesEvent).toBeDefined()
    expect(result.queriesEvent!.queries).toEqual(['query 1', 'query 2'])
  })

  it('should extract thinkingEvents as an array', () => {
    const events = [
      getDeepResearchToolUseEvent(),
      getDeepResearchEvent({
        thinkingEvent: {
          query: 'q1',
          chunksAnalyzed: 5,
          chunksSelected: 2,
          urlsAnalyzed: 3,
          urlsSelected: [],
          urlsInfo: [],
        },
      }),
      getDeepResearchEvent({
        thinkingEvent: {
          query: 'q2',
          chunksAnalyzed: 10,
          chunksSelected: 4,
          urlsAnalyzed: 6,
          urlsSelected: [],
          urlsInfo: [],
        },
      }),
    ]
    const result = extractDeepResearchEvents(events)

    expect(result.thinkingEvents).toHaveLength(2)
    expect(result.thinkingEvents[0].query).toBe('q1')
    expect(result.thinkingEvents[1].query).toBe('q2')
  })

  it('should extract the last progressEvent', () => {
    const events = [
      getDeepResearchToolUseEvent(),
      getDeepResearchEvent({
        progressEvent: {
          elapsedSeconds: 10,
          iterationCount: 1,
          queriesCount: 2,
          urlsAnalyzed: 5,
          snippetsAnalyzed: 10,
        },
      }),
      getDeepResearchEvent({
        progressEvent: {
          elapsedSeconds: 25,
          iterationCount: 2,
          queriesCount: 4,
          urlsAnalyzed: 12,
          snippetsAnalyzed: 24,
        },
      }),
    ]
    const result = extractDeepResearchEvents(events)

    expect(result.progressEvent).toBeDefined()
    expect(result.progressEvent!.elapsedSeconds).toBe(25)
    expect(result.progressEvent!.queriesCount).toBe(4)
  })

  it('should extract completeEvent', () => {
    const events = [
      getDeepResearchToolUseEvent(),
      getDeepResearchEvent({
        completeEvent: { reason: 'finished' },
      }),
    ]
    const result = extractDeepResearchEvents(events)

    expect(result.completeEvent).toBeDefined()
    expect(result.completeEvent!.reason).toBe('finished')
  })

  it('should extract errorEvent', () => {
    const events = [
      getDeepResearchToolUseEvent(),
      getDeepResearchEvent({
        errorEvent: { error: 'timeout' },
      }),
    ]
    const result = extractDeepResearchEvents(events)

    expect(result.errorEvent).toBeDefined()
    expect(result.errorEvent!.error).toBe('timeout')
  })

  it('should extract searchStatusEvent', () => {
    const events = [
      getDeepResearchToolUseEvent(),
      getDeepResearchEvent({
        searchStatusEvent: {
          status: DeepResearchSearchStatus.kStarted,
          query: 'test',
          queryIndex: 0,
          totalQueries: 3,
          urlsFound: 0,
          elapsedMs: 0,
        },
      }),
    ]
    const result = extractDeepResearchEvents(events)

    expect(result.searchStatusEvent).toBeDefined()
    expect(result.searchStatusEvent!.query).toBe('test')
  })

  it('should extract analysisStatusEvent', () => {
    const events = [
      getDeepResearchToolUseEvent(),
      getDeepResearchEvent({
        analysisStatusEvent: {
          status: DeepResearchAnalysisStatus.kProgress,
          query: 'analysis query',
          chunksAnalyzed: 5,
          chunksTotal: 10,
        },
      }),
    ]
    const result = extractDeepResearchEvents(events)

    expect(result.analysisStatusEvent).toBeDefined()
    expect(result.analysisStatusEvent!.chunksAnalyzed).toBe(5)
  })

  it('should extract iterationCompleteEvent', () => {
    const events = [
      getDeepResearchToolUseEvent(),
      getDeepResearchEvent({
        iterationCompleteEvent: {
          iteration: 1,
          totalIterations: 3,
          queriesThisIteration: 5,
          urlsAnalyzed: 10,
          blindspotsIdentified: 2,
        },
      }),
    ]
    const result = extractDeepResearchEvents(events)

    expect(result.iterationCompleteEvent).toBeDefined()
    expect(result.iterationCompleteEvent!.iteration).toBe(1)
  })

  it('should extract analyzingEvent', () => {
    const events = [
      getDeepResearchToolUseEvent(),
      getDeepResearchEvent({
        analyzingEvent: {
          query: 'analyzing query',
          urlCount: 10,
          newUrlCount: 5,
        },
      }),
    ]
    const result = extractDeepResearchEvents(events)

    expect(result.analyzingEvent).toBeDefined()
    expect(result.analyzingEvent!.query).toBe('analyzing query')
  })

  it('should extract fetchStatusEvent', () => {
    const events = [
      getDeepResearchToolUseEvent(),
      getDeepResearchEvent({
        fetchStatusEvent: {
          query: 'fetch query',
          urlsTotal: 10,
          urlsFetched: 5,
        },
      }),
    ]
    const result = extractDeepResearchEvents(events)

    expect(result.fetchStatusEvent).toBeDefined()
    expect(result.fetchStatusEvent!.urlsFetched).toBe(5)
  })

  describe('multiple deep research tasks', () => {
    it('should only use events after the last deep research ToolUseEvent', () => {
      const events = [
        getDeepResearchToolUseEvent(),
        getDeepResearchEvent({
          queriesEvent: { queries: ['old query'] },
        }),
        getDeepResearchEvent({
          completeEvent: { reason: 'first run done' },
        }),
        // Second deep research task
        getDeepResearchToolUseEvent(),
        getDeepResearchEvent({
          queriesEvent: { queries: ['new query'] },
        }),
      ]
      const result = extractDeepResearchEvents(events)

      expect(result.queriesEvent!.queries).toEqual(['new query'])
      // completeEvent from first task should NOT be present
      expect(result.completeEvent).toBeUndefined()
    })

    it('should not include events from earlier deep research tasks', () => {
      const events = [
        getDeepResearchToolUseEvent(),
        getDeepResearchEvent({
          errorEvent: { error: 'first error' },
        }),
        // Second deep research task
        getDeepResearchToolUseEvent(),
        getDeepResearchEvent({
          progressEvent: {
            elapsedSeconds: 5,
            iterationCount: 1,
            queriesCount: 1,
            urlsAnalyzed: 2,
            snippetsAnalyzed: 4,
          },
        }),
      ]
      const result = extractDeepResearchEvents(events)

      expect(result.errorEvent).toBeUndefined()
      expect(result.progressEvent).toBeDefined()
      expect(result.progressEvent!.elapsedSeconds).toBe(5)
    })
  })

  describe('hasSynthesisCompletion', () => {
    it('should be false when no completion event exists', () => {
      const events = [
        getDeepResearchToolUseEvent(),
        getDeepResearchEvent({
          queriesEvent: { queries: ['test'] },
        }),
      ]
      const result = extractDeepResearchEvents(events)

      expect(result.hasSynthesisCompletion).toBe(false)
    })

    it('should be true when a completion event exists after the tool use', () => {
      const events = [
        getDeepResearchToolUseEvent(),
        getDeepResearchEvent({
          completeEvent: { reason: 'done' },
        }),
        getCompletionEvent('Here is the synthesized answer...'),
      ]
      const result = extractDeepResearchEvents(events)

      expect(result.hasSynthesisCompletion).toBe(true)
    })

    it('should not count completion events before the last tool use', () => {
      const events = [
        getCompletionEvent('Pre-tool completion text'),
        getDeepResearchToolUseEvent(),
        getDeepResearchEvent({
          queriesEvent: { queries: ['test'] },
        }),
      ]
      const result = extractDeepResearchEvents(events)

      expect(result.hasSynthesisCompletion).toBe(false)
    })
  })
})
