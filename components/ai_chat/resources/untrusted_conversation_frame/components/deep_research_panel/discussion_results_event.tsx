/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import type { DiscussionResult } from './deep_research_types'
import styles from './enrichments.module.scss'

interface DiscussionResultsProps {
  discussions: DiscussionResult[]
}

const MAX_DISCUSSIONS_DISPLAYED = 5

const DiscussionResultsEvent = React.memo(function DiscussionResultsEvent({ discussions }: DiscussionResultsProps) {
  const [isExpanded, setIsExpanded] = React.useState(false)

  const visibleDiscussions = isExpanded ? discussions : discussions.slice(0, MAX_DISCUSSIONS_DISPLAYED)
  const hiddenCount = Math.max(discussions.length - MAX_DISCUSSIONS_DISPLAYED, 0)

  const handleDiscussionClick = (url: string) => {
    window.open(url, '_blank', 'noopener,noreferrer')
  }

  return (
    <div className={styles.enrichmentSection}>
      <div className={styles.enrichmentHeader}>
        <Icon name='message-bubble-comments' />
        <span>Discussions</span>
      </div>
      <div className={styles.discussionsGrid}>
        {visibleDiscussions.map((discussion, idx) => (
          <button
            key={idx}
            className={styles.discussionCard}
            onClick={() => handleDiscussionClick(discussion.url)}
          >
            <div className={styles.discussionContent}>
              <div className={styles.discussionTitle}>{discussion.title}</div>
              {discussion.description && (
                <div className={styles.discussionDescription}>
                  {discussion.description}
                </div>
              )}
              <div className={styles.discussionMeta}>
                {discussion.favicon && (
                  <img
                    src={discussion.favicon}
                    alt=''
                    className={styles.discussionFavicon}
                  />
                )}
                {discussion.forum_name && (
                  <span className={styles.forumName}>{discussion.forum_name}</span>
                )}
                {discussion.num_answers !== undefined && (
                  <>
                    <span className={styles.metaSeparator}>·</span>
                    <span className={styles.answerCount}>
                      {discussion.num_answers} {discussion.num_answers === 1 ? 'answer' : 'answers'}
                    </span>
                  </>
                )}
                {discussion.age && (
                  <>
                    <span className={styles.metaSeparator}>·</span>
                    <span className={styles.discussionAge}>{discussion.age}</span>
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
          Show {hiddenCount} more discussions
        </button>
      )}
    </div>
  )
})

export default DiscussionResultsEvent
