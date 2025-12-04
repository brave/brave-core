/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'
import MarkdownRenderer from '../markdown_renderer'
import ImageResultsEvent from './image_results_event'
import NewsResultsEvent from './news_results_event'
import DiscussionResultsEvent from './discussion_results_event'
import styles from './style.module.scss'
import type { DeepResearchEvent } from './deep_research_types'

interface EventRendererProps {
  event: DeepResearchEvent
  showThinkingHeader?: boolean
}

const MAX_SOURCES = 3

export const DeepResearchEventRenderer = React.memo(function DeepResearchEventRenderer({ event, showThinkingHeader = false }: EventRendererProps) {
  if (event.type === 'queries') {
    return (
      <div className={styles.deepResearchQueries}>
        <div className={styles.deepResearchQueriesHeader}>
          <Icon name='search' />
          {getLocale(S.CHAT_UI_DEEP_RESEARCH_SEARCH)}
        </div>
        <div className={styles.deepResearchQueriesContentWrapper}>
          <div className={styles.deepResearchQueriesContent}>
            {event.queries.map((query, idx) => (
              <div key={`query-${idx}-${query}`} className={styles.deepResearchQuery}>
                <div className={styles.deepResearchIconWrapper}>
                  <Icon name='search' />
                </div>
                <span>{query}</span>
              </div>
            ))}
          </div>
        </div>
      </div>
    )
  }

  if (event.type === 'thinking') {
    const [isExpanded, setIsExpanded] = React.useState(false)
    const extraSources = Math.max((event.urls_info?.length ?? 0) - MAX_SOURCES, 0)
    const visibleSources = isExpanded ? event.urls_info : event.urls_info.slice(0, MAX_SOURCES)

    return (
      <div className={styles.deepResearchThinking}>
        {showThinkingHeader && (
          <div className={styles.deepResearchThinkingHeader}>
            <Icon name='product-chip-outline' />
            {getLocale(S.CHAT_UI_DEEP_RESEARCH_THINK)}
          </div>
        )}
        <div className={styles.deepResearchThinkingContentWrapper}>
          <div className={styles.deepResearchThinkingContent}>
            <div className={styles.deepResearchQuery}>
              <div className={styles.deepResearchIconWrapper}>
                <Icon name='search' />
              </div>
              <span>{event.query}</span>
            </div>
            <div className={styles.deepResearchThinkingInnerWrapper}>
              <div className={styles.deepResearchThinkingInner}>
                <div className={styles.deepResearchQuery}>
                  <div className={styles.deepResearchIconWrapper}>
                    <Icon name='link-normal' />
                  </div>
                  <span className={styles.tertiary}>
                    Found {event.urls_analyzed} total sources
                  </span>
                </div>
                {event.urls_info?.length > 0 && (
                  <>
                    <div className={styles.deepResearchQuery}>
                      <div className={styles.deepResearchIconWrapper}>
                        <Icon name='product-chip-outline' />
                      </div>
                      <span className={styles.tertiary}>
                        Analyzing the {event.urls_info.length} most relevant sources:
                      </span>
                    </div>
                    <div className={styles.deepResearchThinkingRelevantSources}>
                      {visibleSources.map((urlInfo, idx) => {
                        const hostname = new URL(urlInfo.url).hostname
                        return (
                          <a
                            key={idx}
                            className={styles.deepResearchSourceChip}
                            href={urlInfo.url}
                            target='_blank'
                            rel='noopener noreferrer'
                          >
                            <div className={styles.deepResearchSourceChipFavicon}>
                              <img src={urlInfo.favicon || 'chrome-untrusted://resources/brave-icons/globe.svg'} alt='' />
                            </div>
                            <span className={styles.tertiary}>{hostname}</span>
                          </a>
                        )
                      })}
                      {extraSources > 0 && !isExpanded && (
                        <button
                          type='button'
                          className={styles.deepResearchSourceChip}
                          onClick={() => setIsExpanded(true)}
                        >
                          <span className={styles.tertiary}>+ {extraSources} more</span>
                        </button>
                      )}
                    </div>
                  </>
                )}
              </div>
            </div>
          </div>
        </div>
      </div>
    )
  }

  if (event.type === 'answer') {
    return (
      <div className={styles.deepResearchAnswer}>
        <div className={styles.deepResearchAnswerHeader}>
          <Icon name='search' />
          {event.final ? getLocale(S.CHAT_UI_DEEP_RESEARCH_ANSWER_OUTLINE) : getLocale(S.CHAT_UI_DEEP_RESEARCH_EVALUATING_ANSWER)}
        </div>
        <div className={styles.deepResearchAnswerContentWrapper}>
          <div className={styles.deepResearchAnswerContent}>
            <MarkdownRenderer text={event.answer} shouldShowTextCursor={false} />
          </div>
        </div>
      </div>
    )
  }

  if (event.type === 'blindspots') {
    return (
      <div className={styles.deepResearchBlindspots}>
        <div className={styles.deepResearchBlindspotsHeader}>
          <Icon name='eye-off' />
          {getLocale(S.CHAT_UI_DEEP_RESEARCH_BLINDSPOTS)}
        </div>
        {event.blindspots.map((blindspot, idx) => (
          <div key={idx} className={styles.deepResearchBlindspotsContentWrapper}>
            <div className={styles.deepResearchBlindspotsContent}>
              <div className={styles.deepResearchQuery}>
                <span>{blindspot}</span>
              </div>
            </div>
          </div>
        ))}
      </div>
    )
  }

  if (event.type === 'progress') {
    const formatDuration = (seconds: number) => {
      if (seconds < 60) return `${Math.round(seconds)}s`
      const minutes = Math.floor(seconds / 60)
      const remainingSeconds = Math.round(seconds % 60)
      return `${minutes}m ${remainingSeconds}s`
    }

    return (
      <div className={styles.deepResearchProgress}>
        <Icon name='clock' />
        <span>
          Researched for {formatDuration(event.elapsed_seconds)}
        </span>
      </div>
    )
  }

  if (event.type === 'images') {
    return <ImageResultsEvent images={event.images} />
  }

  if (event.type === 'news') {
    return <NewsResultsEvent news={event.news} />
  }

  if (event.type === 'discussions') {
    return <DiscussionResultsEvent discussions={event.discussions} />
  }

  return null
})
