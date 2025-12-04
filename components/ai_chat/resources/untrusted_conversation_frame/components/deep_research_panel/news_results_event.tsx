/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import type { NewsResult } from './deep_research_types'
import styles from './enrichments.module.scss'

interface NewsResultsProps {
  news: NewsResult[]
}

const MAX_NEWS_DISPLAYED = 5

const NewsResultsEvent = React.memo(function NewsResultsEvent({ news }: NewsResultsProps) {
  const [isExpanded, setIsExpanded] = React.useState(false)

  const visibleNews = isExpanded ? news : news.slice(0, MAX_NEWS_DISPLAYED)
  const hiddenCount = Math.max(news.length - MAX_NEWS_DISPLAYED, 0)

  const handleNewsClick = (url: string) => {
    window.open(url, '_blank', 'noopener,noreferrer')
  }

  return (
    <div className={styles.enrichmentSection}>
      <div className={styles.enrichmentHeader}>
        <Icon name='news' />
        <span>News</span>
      </div>
      <div className={styles.newsGrid}>
        {visibleNews.map((newsItem, idx) => (
          <button
            key={idx}
            className={styles.newsCard}
            onClick={() => handleNewsClick(newsItem.url)}
          >
            {newsItem.thumbnail_url && (
              <div className={styles.newsThumbnail}>
                <img
                  src={newsItem.thumbnail_url}
                  alt={newsItem.title}
                  loading='lazy'
                />
                {newsItem.is_breaking && (
                  <span className={styles.breakingBadge}>BREAKING</span>
                )}
              </div>
            )}
            <div className={styles.newsContent}>
              <div className={styles.newsTitle}>{newsItem.title}</div>
              <div className={styles.newsMeta}>
                {newsItem.favicon && (
                  <img
                    src={newsItem.favicon}
                    alt=''
                    className={styles.newsFavicon}
                  />
                )}
                <span className={styles.newsSource}>{newsItem.source}</span>
                {newsItem.age && (
                  <>
                    <span className={styles.metaSeparator}>·</span>
                    <span className={styles.newsAge}>{newsItem.age}</span>
                  </>
                )}
              </div>
            </div>
          </button>
        ))}
      </div>
      {hiddenCount > 0 && !isExpanded && (
        <button
          className={styles.expandButton}
          onClick={() => setIsExpanded(true)}
        >
          <Icon name='plus-add' />
          Show {hiddenCount} more articles
        </button>
      )}
    </div>
  )
})

export default NewsResultsEvent
