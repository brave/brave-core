// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import { getLocale, formatLocale } from '$web-common/locale'
import * as mojom from '../../../common/mojom'
import styles from './deep_research_event.module.scss'

interface DeepResearchEventProps {
  queriesEvent?: mojom.DeepResearchQueriesEvent
  thinkingEvents: mojom.DeepResearchThinkingEvent[]
  progressEvent?: mojom.DeepResearchProgressEvent
  completeEvent?: mojom.DeepResearchCompleteEvent
  errorEvent?: mojom.DeepResearchErrorEvent
  searchStatusEvent?: mojom.DeepResearchSearchStatusEvent
  analysisStatusEvent?: mojom.DeepResearchAnalysisStatusEvent
  iterationCompleteEvent?: mojom.DeepResearchIterationCompleteEvent
  analyzingEvent?: mojom.DeepResearchAnalyzingEvent
  fetchStatusEvent?: mojom.DeepResearchFetchStatusEvent
  isActive: boolean
}

interface LastEventInfo {
  description: string
  iconName: string
}

function getLastEventInfo(props: DeepResearchEventProps): LastEventInfo | null {
  // Priority order: Active/in-progress events first, then completed events
  // Events are checked for "active" status to avoid stale events dominating

  // Search in progress (not completed yet)
  if (
    props.searchStatusEvent
    && props.searchStatusEvent.status
      === mojom.DeepResearchSearchStatus.kStarted
  ) {
    const e = props.searchStatusEvent
    return {
      description: formatLocale(S.CHAT_UI_DEEP_RESEARCH_SEARCHING, {
        $1: e.query,
      }),
      iconName: 'search',
    }
  }

  // LLM analysis in progress - only show if actively analyzing (not completed)
  if (
    props.analysisStatusEvent
    && props.analysisStatusEvent.chunksAnalyzed
      < props.analysisStatusEvent.chunksTotal
  ) {
    const e = props.analysisStatusEvent
    return {
      description: formatLocale(S.CHAT_UI_DEEP_RESEARCH_ANALYZING_SOURCES, {
        $1: String(e.chunksTotal),
        $2: e.query,
      }),
      iconName: 'file',
    }
  }

  // Analyzing URLs (processing search results)
  if (props.analyzingEvent) {
    const e = props.analyzingEvent
    return {
      description: formatLocale(S.CHAT_UI_DEEP_RESEARCH_PROCESSING_URLS, {
        $1: String(e.newUrlCount),
        $2: e.query,
      }),
      iconName: 'search',
    }
  }

  // Search completed - shows URLs found
  if (
    props.searchStatusEvent
    && props.searchStatusEvent.status
      === mojom.DeepResearchSearchStatus.kCompleted
  ) {
    const e = props.searchStatusEvent
    return {
      description: formatLocale(S.CHAT_UI_DEEP_RESEARCH_FOUND_URLS, {
        $1: String(e.urlsFound),
        $2: e.query,
      }),
      iconName: 'search',
    }
  }

  // URL fetching in progress
  if (props.fetchStatusEvent) {
    const e = props.fetchStatusEvent
    return {
      description: formatLocale(S.CHAT_UI_DEEP_RESEARCH_FETCHING_URLS, {
        $1: String(e.urlsFetched),
        $2: String(e.urlsTotal),
      }),
      iconName: 'search',
    }
  }

  // Thinking event = analysis complete for a query (lower priority - it accumulates)
  if (props.thinkingEvents && props.thinkingEvents.length > 0) {
    const e = props.thinkingEvents[props.thinkingEvents.length - 1]
    return {
      description: formatLocale(S.CHAT_UI_DEEP_RESEARCH_ANALYZED_URLS, {
        $1: String(e.urlsAnalyzed),
        $2: e.query,
      }),
      iconName: 'file',
    }
  }

  // Queries event = search queries generated
  if (props.queriesEvent && props.queriesEvent.queries.length > 0) {
    const queries = props.queriesEvent.queries
    return {
      description: formatLocale(S.CHAT_UI_DEEP_RESEARCH_SEARCHING_QUERIES, {
        $1: String(queries.length),
      }),
      iconName: 'search',
    }
  }

  // Progress event = fallback showing overall stats
  if (props.progressEvent) {
    const e = props.progressEvent
    return {
      description: formatLocale(S.CHAT_UI_DEEP_RESEARCH_PROGRESS, {
        $1: String(e.urlsAnalyzed),
        $2: String(e.queriesCount),
      }),
      iconName: 'search',
    }
  }

  // Complete event = research finished
  if (props.completeEvent) {
    return {
      description: getLocale(S.CHAT_UI_DEEP_RESEARCH_COMPLETE),
      iconName: 'check-circle-filled',
    }
  }

  return null
}

function ElapsedTimeCounter(props: {
  progressEvent?: mojom.DeepResearchProgressEvent
}) {
  const [elapsed, setElapsed] = React.useState(0)
  const startTimeRef = React.useRef<number | null>(null)

  // Calculate start time from progressEvent.elapsedSeconds when available
  React.useEffect(() => {
    if (props.progressEvent && !startTimeRef.current) {
      startTimeRef.current =
        Date.now() - props.progressEvent.elapsedSeconds * 1000
    } else if (!startTimeRef.current) {
      startTimeRef.current = Date.now()
    }
  }, [props.progressEvent])

  React.useEffect(() => {
    const updateElapsed = () => {
      if (startTimeRef.current) {
        const elapsedSeconds = Math.floor(
          (Date.now() - startTimeRef.current) / 1000,
        )
        setElapsed(elapsedSeconds)
      }
    }

    updateElapsed()
    const interval = setInterval(updateElapsed, 1000)

    return () => clearInterval(interval)
  }, [])

  const minutes = Math.floor(elapsed / 60)
  const seconds = elapsed % 60

  const timeDisplay =
    minutes > 0
      ? formatLocale(S.CHAT_UI_DEEP_RESEARCH_ELAPSED_MINUTES_SECONDS, {
          $1: String(minutes),
          $2: String(seconds),
        })
      : formatLocale(S.CHAT_UI_DEEP_RESEARCH_ELAPSED_SECONDS, {
          $1: String(seconds),
        })

  return <span className={styles.elapsed}>{timeDisplay}</span>
}

function DeepResearchError(props: {
  errorEvent: mojom.DeepResearchErrorEvent
}) {
  return (
    <div className={styles.error}>
      <Icon name='warning-triangle-filled' />
      <span>
        {formatLocale(S.CHAT_UI_DEEP_RESEARCH_ERROR, {
          $1: props.errorEvent.error,
        })}
      </span>
    </div>
  )
}

function DeepResearchProgressLine(props: DeepResearchEventProps) {
  const currentEventInfo = getLastEventInfo(props)
  const [displayedInfo, setDisplayedInfo] =
    React.useState<LastEventInfo | null>(currentEventInfo)
  const [isFading, setIsFading] = React.useState(false)
  const lastUpdateTimeRef = React.useRef<number>(Date.now())
  const eventQueueRef = React.useRef<LastEventInfo[]>([])
  const processingRef = React.useRef<boolean>(false)

  // Memoize the description to detect changes
  const currentDescription = currentEventInfo?.description

  // Process the event queue
  const processQueue = React.useCallback(() => {
    if (processingRef.current || eventQueueRef.current.length === 0) {
      return
    }

    const MIN_DISPLAY_TIME = 1000 // 1 second minimum
    const FADE_DURATION = 150

    const timeSinceLastUpdate = Date.now() - lastUpdateTimeRef.current
    const waitTime = Math.max(0, MIN_DISPLAY_TIME - timeSinceLastUpdate)

    processingRef.current = true

    setTimeout(() => {
      // Get the most recent event in queue (skip intermediate ones)
      const nextEvent = eventQueueRef.current[eventQueueRef.current.length - 1]
      eventQueueRef.current = []

      if (!nextEvent) {
        processingRef.current = false
        return
      }

      // Fade out
      setIsFading(true)

      // After fade out, update content and fade in
      setTimeout(() => {
        setDisplayedInfo(nextEvent)
        lastUpdateTimeRef.current = Date.now()
        setIsFading(false)
        processingRef.current = false

        // Check if more events arrived while we were animating
        processQueue()
      }, FADE_DURATION)
    }, waitTime)
  }, [])

  React.useEffect(() => {
    if (!currentEventInfo) return
    if (currentDescription === displayedInfo?.description) return

    // Add to queue instead of cancelling previous
    eventQueueRef.current.push(currentEventInfo)

    // Start processing if not already
    processQueue()
  }, [
    currentDescription,
    currentEventInfo,
    displayedInfo?.description,
    processQueue,
  ])

  if (!displayedInfo) return null

  return (
    <div className={styles.progressLine}>
      <Icon
        name={displayedInfo.iconName}
        className={styles.icon}
      />
      <span
        className={`${styles.description} ${isFading ? styles.fadeOut : styles.fadeIn}`}
      >
        {displayedInfo.description}
      </span>
      <ElapsedTimeCounter progressEvent={props.progressEvent} />
    </div>
  )
}

export default function DeepResearchEvent(props: DeepResearchEventProps) {
  const { completeEvent, errorEvent, isActive } = props

  // Show progress line if active OR if we have ongoing events but no completion yet
  const showProgress = isActive || (!completeEvent && !errorEvent)

  return (
    <div className={styles.deepResearch}>
      {/* Single-line progress while active */}
      {showProgress && (
        <DeepResearchProgressLine
          queriesEvent={props.queriesEvent}
          thinkingEvents={props.thinkingEvents}
          progressEvent={props.progressEvent}
          completeEvent={props.completeEvent}
          errorEvent={props.errorEvent}
          searchStatusEvent={props.searchStatusEvent}
          analysisStatusEvent={props.analysisStatusEvent}
          iterationCompleteEvent={props.iterationCompleteEvent}
          analyzingEvent={props.analyzingEvent}
          fetchStatusEvent={props.fetchStatusEvent}
          isActive={props.isActive}
        />
      )}

      {/* Error message if any */}
      {errorEvent && <DeepResearchError errorEvent={errorEvent} />}

      {/* Completion status (when not active) */}
      {completeEvent && !isActive && (
        <div className={styles.complete}>
          <Icon name='check-circle-filled' />
          <span>{getLocale(S.CHAT_UI_DEEP_RESEARCH_COMPLETE)}</span>
        </div>
      )}
    </div>
  )
}
