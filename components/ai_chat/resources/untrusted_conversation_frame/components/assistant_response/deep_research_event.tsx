// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
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

type EventCategory = 'search' | 'analysis' | 'progress' | 'complete'

interface LastEventInfo {
  category: EventCategory
  description: string
  iconName: string
}

function getLastEventInfo(props: DeepResearchEventProps): LastEventInfo | null {
  // Priority order: Active/in-progress events first, then completed events
  // Events are checked for "active" status to avoid stale events dominating

  // Search in progress (not completed yet)
  if (props.searchStatusEvent && props.searchStatusEvent.status === 'started') {
    const e = props.searchStatusEvent
    const shortQuery = e.query.length > 30 ? e.query.slice(0, 30) + '...' : e.query
    return {
      category: 'search',
      description: `Searching: "${shortQuery}"`,
      iconName: 'search'
    }
  }

  // LLM analysis in progress - only show if actively analyzing (not completed)
  if (props.analysisStatusEvent && props.analysisStatusEvent.chunksAnalyzed < props.analysisStatusEvent.chunksTotal) {
    const e = props.analysisStatusEvent
    const shortQuery = e.query.length > 40 ? e.query.slice(0, 40) + '...' : e.query
    return {
      category: 'analysis',
      description: `Analyzing ${e.chunksTotal} sources for "${shortQuery}"`,
      iconName: 'file'
    }
  }

  // Analyzing URLs (processing search results)
  if (props.analyzingEvent) {
    const e = props.analyzingEvent
    const shortQuery = e.query.length > 40 ? e.query.slice(0, 40) + '...' : e.query
    return {
      category: 'search',
      description: `Processing ${e.newUrls} new URLs for "${shortQuery}"`,
      iconName: 'search'
    }
  }

  // Search completed - shows URLs found
  if (props.searchStatusEvent && props.searchStatusEvent.status === 'completed') {
    const e = props.searchStatusEvent
    const shortQuery = e.query.length > 30 ? e.query.slice(0, 30) + '...' : e.query
    return {
      category: 'search',
      description: `Found ${e.urlsFound} URLs for "${shortQuery}"`,
      iconName: 'search'
    }
  }

  // URL fetching in progress
  if (props.fetchStatusEvent) {
    const e = props.fetchStatusEvent
    return {
      category: 'search',
      description: `Fetching ${e.urlsFetched}/${e.urlsTotal} URLs`,
      iconName: 'search'
    }
  }

  // Thinking event = analysis complete for a query (lower priority - it accumulates)
  if (props.thinkingEvents && props.thinkingEvents.length > 0) {
    const e = props.thinkingEvents[props.thinkingEvents.length - 1]
    const shortQuery = e.query.length > 40 ? e.query.slice(0, 40) + '...' : e.query
    return {
      category: 'analysis',
      description: `Analyzed ${e.urlsAnalyzed} URLs for "${shortQuery}"`,
      iconName: 'file'
    }
  }

  // Queries event = search queries generated
  if (props.queriesEvent && props.queriesEvent.queries.length > 0) {
    const queries = props.queriesEvent.queries
    return {
      category: 'search',
      description: `Searching ${queries.length} ${queries.length === 1 ? 'query' : 'queries'}`,
      iconName: 'search'
    }
  }

  // Progress event = fallback showing overall stats
  if (props.progressEvent) {
    const e = props.progressEvent
    return {
      category: 'progress',
      description: `Analyzed ${e.urlsAnalyzed} URLs from ${e.queriesCount} searches`,
      iconName: 'search'
    }
  }

  // Complete event = research finished
  if (props.completeEvent) {
    return {
      category: 'complete',
      description: 'Research complete',
      iconName: 'check-circle-filled'
    }
  }

  return null
}

function ElapsedTimeCounter(props: { progressEvent?: mojom.DeepResearchProgressEvent }) {
  const [elapsed, setElapsed] = React.useState(0)
  const startTimeRef = React.useRef<number | null>(null)

  // Calculate start time from progressEvent.elapsedSeconds when available
  React.useEffect(() => {
    if (props.progressEvent && !startTimeRef.current) {
      startTimeRef.current = Date.now() - (props.progressEvent.elapsedSeconds * 1000)
    } else if (!startTimeRef.current) {
      startTimeRef.current = Date.now()
    }
  }, [props.progressEvent])

  React.useEffect(() => {
    const updateElapsed = () => {
      if (startTimeRef.current) {
        const elapsedSeconds = Math.floor((Date.now() - startTimeRef.current) / 1000)
        setElapsed(elapsedSeconds)
      }
    }

    updateElapsed()
    const interval = setInterval(updateElapsed, 1000)

    return () => clearInterval(interval)
  }, [])

  const minutes = Math.floor(elapsed / 60)
  const seconds = elapsed % 60

  return (
    <span className={styles.elapsed}>
      {minutes > 0 ? `${minutes}m ` : ''}{seconds}s
    </span>
  )
}

function DeepResearchError(props: { errorEvent: mojom.DeepResearchErrorEvent }) {
  return (
    <div className={styles.error}>
      <Icon name='warning-triangle-filled' />
      <span>Research error: {props.errorEvent.error}</span>
    </div>
  )
}

function DeepResearchProgressLine(props: DeepResearchEventProps) {
  const currentEventInfo = getLastEventInfo(props)
  const [displayedInfo, setDisplayedInfo] = React.useState<LastEventInfo | null>(currentEventInfo)
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
  }, [currentDescription, currentEventInfo, displayedInfo?.description, processQueue])

  if (!displayedInfo) return null

  return (
    <div className={styles.progressLine}>
      <Icon name={displayedInfo.iconName} className={styles.icon} />
      <span className={`${styles.description} ${isFading ? styles.fadeOut : styles.fadeIn}`}>
        {displayedInfo.description}
      </span>
      <ElapsedTimeCounter progressEvent={props.progressEvent} />
    </div>
  )
}

export default function DeepResearchEvent(props: DeepResearchEventProps) {
  const {
    completeEvent,
    errorEvent,
    isActive,
  } = props

  // Show progress line if active OR if we have ongoing events but no completion yet
  const showProgress = isActive || (!completeEvent && !errorEvent)

  return (
    <div className={styles.deepResearch}>
      {/* Single-line progress while active */}
      {showProgress && <DeepResearchProgressLine {...props} />}

      {/* Error message if any */}
      {errorEvent && <DeepResearchError errorEvent={errorEvent} />}

      {/* Completion status (when not active) */}
      {completeEvent && !isActive && (
        <div className={styles.complete}>
          <Icon name='check-circle-filled' />
          <span>Research complete</span>
        </div>
      )}
    </div>
  )
}
