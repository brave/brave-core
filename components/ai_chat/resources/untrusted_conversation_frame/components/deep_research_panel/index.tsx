/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'
import { DeepResearchEventRenderer } from './deep_research_event'
import styles from './style.module.scss'
import type { DeepResearchMessage } from './deep_research_types'

interface DeepResearchPanelProps {
  message: DeepResearchMessage
}

const DeepResearchPanel = React.memo(function DeepResearchPanel({ message }: DeepResearchPanelProps) {
  const [expanded, setExpanded] = React.useState(false)
  const [elapsedTime, setElapsedTime] = React.useState(0)

  const HeaderIcon = message.finished ? 'check-normal' : 'loading'
  const ExpandIcon = expanded ? 'arrow-small-up' : 'arrow-small-down'

  // Calculate stats from events
  const stats = React.useMemo(() => {
    const urlsAnalyzed = message.events
      .filter(e => e.type === 'thinking')
      .reduce((acc, e) => acc + (e.type === 'thinking' ? e.urls_analyzed : 0), 0)

    const queriesIssued = message.events
      .filter(e => e.type === 'queries')
      .reduce((acc, e) => acc + (e.type === 'queries' ? e.queries.length : 0), 0)

    const lastProgressEvent = message.events
      .filter(e => e.type === 'progress')
      .pop()

    // Use the maximum of backend time and local timer to avoid freezing
    const elapsed = Math.max(
      lastProgressEvent?.elapsed_seconds || 0,
      elapsedTime
    )

    return {
      urlsAnalyzed,
      queriesIssued,
      elapsed
    }
  }, [message.events, elapsedTime])

  // Real-time elapsed timer
  React.useEffect(() => {
    if (message.finished) return

    const interval = setInterval(() => {
      setElapsedTime(prev => prev + 1)
    }, 1000)

    return () => clearInterval(interval)
  }, [message.finished])

  // Get last non-ping event for collapsed view
  const lastEvent = React.useMemo(() => {
    return message.events
      .filter(e => e.type !== 'analyzing')
      .reverse()
      .find(e =>
        e.type === 'queries' ||
        e.type === 'thinking' ||
        e.type === 'answer' ||
        e.type === 'blindspots'
      )
  }, [message.events])

  const formatDuration = (seconds: number) => {
    if (seconds < 60) return `${Math.round(seconds)}s`
    const minutes = Math.floor(seconds / 60)
    const remainingSeconds = Math.round(seconds % 60)
    return `${minutes}m ${remainingSeconds}s`
  }

  return (
    <div className={`${styles.deepResearch} ${expanded ? styles.expanded : ''}`}>
      <div className={styles.deepResearchSticky}>
        <div className={styles.deepResearchHeader}>
          <div className={styles.deepResearchHeaderPrimary}>
            <Icon name={HeaderIcon} />
            {getLocale(S.CHAT_UI_DEEP_RESEARCH)}
            <button
              className={styles.deepResearchExpandToggle}
              onClick={() => setExpanded(!expanded)}
              aria-label={expanded ? 'Collapse' : 'Expand'}
            >
              <Icon name={ExpandIcon} />
            </button>
          </div>
        </div>

        <div className={styles.deepResearchStats}>
          <div className={styles.deepResearchStatsItem}>
            <div className={styles.deepResearchStatsItemValue}>
              {stats.urlsAnalyzed || '-'}
            </div>
            <div className={styles.deepResearchStatsItemLabel}>
              <Icon name='link-normal' />
              <span>{getLocale(S.CHAT_UI_DEEP_RESEARCH_URLS_ANALYZED)}</span>
            </div>
          </div>
          <div className={styles.deepResearchStatsItem}>
            <div className={styles.deepResearchStatsItemValue}>
              {stats.queriesIssued || '-'}
            </div>
            <div className={styles.deepResearchStatsItemLabel}>
              <Icon name='search' />
              <span>{getLocale(S.CHAT_UI_DEEP_RESEARCH_QUERIES_ISSUED)}</span>
            </div>
          </div>
          <div className={styles.deepResearchStatsItem}>
            <div className={styles.deepResearchStatsItemValue}>
              {formatDuration(stats.elapsed)}
            </div>
            <div className={styles.deepResearchStatsItemLabel}>
              <Icon name='clock' />
              <span>{getLocale(S.CHAT_UI_DEEP_RESEARCH_ELAPSED)}</span>
            </div>
          </div>
        </div>
      </div>

      {!expanded && lastEvent ? (
        <div className={styles.deepResearchLastEvent}>
          <DeepResearchEventRenderer event={lastEvent} showThinkingHeader={true} />
        </div>
      ) : (
        <div className={styles.deepResearchContent}>
          {message.events
            .filter(e => e.type !== 'analyzing' && e.type !== 'progress')
            .map((event, idx) => {
              // Determine if thinking header should be shown
              const showThinkingHeader = event.type === 'thinking' &&
                message.events.slice(0, idx).every(e => e.type !== 'thinking')

              return (
                <DeepResearchEventRenderer
                  key={`${event.type}-${idx}`}
                  event={event}
                  showThinkingHeader={showThinkingHeader}
                />
              )
            })}
        </div>
      )}
    </div>
  )
})

export default DeepResearchPanel
