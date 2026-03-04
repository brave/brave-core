// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Set up loadTimeData mock BEFORE importing components
;(window as any).loadTimeData = {
  getString: jest.fn((key: string) => `[${key}]`),
  getStringF: jest.fn((_key: string, ...args: string[]) => args.join(', ')),
  getInteger: jest.fn().mockReturnValue(0),
  getBoolean: jest.fn().mockReturnValue(false),
  set: jest.fn(),
  data_: {},
}

import * as React from 'react'
import '@testing-library/jest-dom'
import { render, act } from '@testing-library/react'
import { ExtractedDeepResearchEvents } from './deep_research_utils'
import DeepResearchEvent from './deep_research_event'

// Define enum values locally to avoid dependency on generated mojom build
// output, which may be stale or missing on CI/cross-platform environments.
// Values must match common.mojom DeepResearchSearchStatus enum ordering.
const DeepResearchSearchStatus = { kStarted: 0, kCompleted: 1 } as const

function createDeepResearch(
  overrides: Partial<ExtractedDeepResearchEvents> = {},
): ExtractedDeepResearchEvents {
  return {
    thinkingEvents: [],
    hasDeepResearchEvents: true,
    hasSynthesisCompletion: false,
    ...overrides,
  }
}

describe('DeepResearchEvent', () => {
  beforeEach(() => {
    jest.useFakeTimers()
  })

  afterEach(() => {
    jest.useRealTimers()
  })

  describe('progress vs completion vs error states', () => {
    it('should show progress line when active', () => {
      const { container } = render(
        <DeepResearchEvent
          deepResearch={createDeepResearch({
            searchStatusEvent: {
              status: DeepResearchSearchStatus.kStarted,
              query: 'test query',
              queryIndex: 0,
              totalQueries: 3,
              urlsFound: 0,
              elapsedMs: 0,
            },
          })}
          isActive={true}
        />,
      )

      const progressLine = container.querySelector('[class*="progressLine"]')
      expect(progressLine).toBeInTheDocument()
      // Should not show completion
      const complete = container.querySelector('[class*="complete"]')
      expect(complete).not.toBeInTheDocument()
    })

    it('should show error when errorEvent is present', () => {
      const { container } = render(
        <DeepResearchEvent
          deepResearch={createDeepResearch({
            errorEvent: { error: 'Something went wrong' },
          })}
          isActive={false}
        />,
      )

      const error = container.querySelector('[class*="error"]')
      expect(error).toBeInTheDocument()
    })

    it('should show completion when completeEvent is present and not active', () => {
      const { container } = render(
        <DeepResearchEvent
          deepResearch={createDeepResearch({
            completeEvent: { reason: 'All done' },
          })}
          isActive={false}
        />,
      )

      const complete = container.querySelector('[class*="complete"]')
      expect(complete).toBeInTheDocument()
    })

    it('should not show completion when active', () => {
      const { container } = render(
        <DeepResearchEvent
          deepResearch={createDeepResearch({
            completeEvent: { reason: 'All done' },
          })}
          isActive={true}
        />,
      )

      const complete = container.querySelector('[class*="complete"]')
      expect(complete).not.toBeInTheDocument()
    })

    it('should show progress when active even with completeEvent', () => {
      const { container } = render(
        <DeepResearchEvent
          deepResearch={createDeepResearch({
            completeEvent: { reason: 'All done' },
            progressEvent: {
              elapsedSeconds: 30,
              iterationCount: 1,
              queriesCount: 3,
              urlsAnalyzed: 10,
              snippetsAnalyzed: 20,
            },
          })}
          isActive={true}
        />,
      )

      const progressLine = container.querySelector('[class*="progressLine"]')
      expect(progressLine).toBeInTheDocument()
    })
  })

  describe('event queue for progress line', () => {
    it('should display initial status immediately', () => {
      const { container } = render(
        <DeepResearchEvent
          deepResearch={createDeepResearch({
            searchStatusEvent: {
              status: DeepResearchSearchStatus.kStarted,
              query: 'initial query',
              queryIndex: 0,
              totalQueries: 3,
              urlsFound: 0,
              elapsedMs: 0,
            },
          })}
          isActive={true}
        />,
      )

      const description = container.querySelector('[class*="description"]')
      expect(description).toBeInTheDocument()
    })

    it('should transition to next status after minimum display time', () => {
      const { container, rerender } = render(
        <DeepResearchEvent
          deepResearch={createDeepResearch({
            searchStatusEvent: {
              status: DeepResearchSearchStatus.kStarted,
              query: 'first query',
              queryIndex: 0,
              totalQueries: 3,
              urlsFound: 0,
              elapsedMs: 0,
            },
          })}
          isActive={true}
        />,
      )

      // Update to a new status
      rerender(
        <DeepResearchEvent
          deepResearch={createDeepResearch({
            searchStatusEvent: {
              status: DeepResearchSearchStatus.kCompleted,
              query: 'first query',
              queryIndex: 0,
              totalQueries: 3,
              urlsFound: 5,
              elapsedMs: 1200,
            },
          })}
          isActive={true}
        />,
      )

      // Advance past the minimum display time (1000ms) + fade duration (150ms)
      act(() => {
        jest.advanceTimersByTime(1200)
      })

      // The description element should still exist (component didn't crash)
      const description = container.querySelector('[class*="description"]')
      expect(description).toBeInTheDocument()
    })
  })

  describe('unmount safety', () => {
    it('should not error when unmounted while queue is processing', () => {
      const consoleSpy = jest.spyOn(console, 'error').mockImplementation()

      const { rerender, unmount } = render(
        <DeepResearchEvent
          deepResearch={createDeepResearch({
            searchStatusEvent: {
              status: DeepResearchSearchStatus.kStarted,
              query: 'test',
              queryIndex: 0,
              totalQueries: 3,
              urlsFound: 0,
              elapsedMs: 0,
            },
          })}
          isActive={true}
        />,
      )

      // Trigger a status change to enqueue an update
      rerender(
        <DeepResearchEvent
          deepResearch={createDeepResearch({
            analyzingEvent: {
              query: 'new query',
              urlCount: 10,
              newUrlCount: 5,
            },
          })}
          isActive={true}
        />,
      )

      // Unmount before the queue finishes processing
      unmount()

      // Advance timers past the queue processing time
      act(() => {
        jest.advanceTimersByTime(2000)
      })

      // Check that no React "state update on unmounted component" errors
      // were logged. Filter for React-specific warnings only.
      const reactErrors = consoleSpy.mock.calls.filter(
        (call) =>
          typeof call[0] === 'string'
          && call[0].includes("Can't perform a React state update"),
      )
      expect(reactErrors).toHaveLength(0)

      consoleSpy.mockRestore()
    })
  })

  describe('elapsed time counter', () => {
    it('should render elapsed time', () => {
      const { container } = render(
        <DeepResearchEvent
          deepResearch={createDeepResearch({
            progressEvent: {
              elapsedSeconds: 30,
              iterationCount: 1,
              queriesCount: 3,
              urlsAnalyzed: 10,
              snippetsAnalyzed: 20,
            },
          })}
          isActive={true}
        />,
      )

      const elapsed = container.querySelector('[class*="elapsed"]')
      expect(elapsed).toBeInTheDocument()
    })

    it('should call setInterval to update elapsed time', () => {
      const setIntervalSpy = jest.spyOn(global, 'setInterval')

      render(
        <DeepResearchEvent
          deepResearch={createDeepResearch({
            progressEvent: {
              elapsedSeconds: 0,
              iterationCount: 0,
              queriesCount: 1,
              urlsAnalyzed: 0,
              snippetsAnalyzed: 0,
            },
          })}
          isActive={true}
        />,
      )

      // Verify that a 1-second interval was created for the timer
      const intervalCalls = setIntervalSpy.mock.calls.filter(
        (call) => call[1] === 1000,
      )
      expect(intervalCalls.length).toBeGreaterThan(0)

      setIntervalSpy.mockRestore()
    })
  })
})
