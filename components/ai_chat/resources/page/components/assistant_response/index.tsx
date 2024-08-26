// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import ProgressRing from '@brave/leo/react/progressRing'
import Icon from '@brave/leo/react/icon'
import formatMessage from '$web-common/formatMessage'
import { getLocale } from '$web-common/locale'
import { Url } from 'gen/url/mojom/url.mojom.m.js'
import * as mojom from '../../api/'
import { useAIChat } from '../../state/ai_chat_context'
import MarkdownRenderer from '../markdown_renderer'
import styles from './style.module.scss'

function SearchSummary (props: { searchQueries: string[] }) {
  const context = useAIChat()

  const handleOpenSearchQuery = React.useCallback((e: React.MouseEvent, query: string) => {
    e.preventDefault()
    const queryUrl = new Url()
    queryUrl.url = `https://search.brave.com/search?q=${encodeURIComponent(query)}`
    context.uiHandler?.openURL(queryUrl)
  }, [])

  const handleLearnMore = () => {
    context.uiHandler?.openLearnMoreAboutBraveSearchWithLeo()
  }

  const message = formatMessage(getLocale('searchQueries'), {
    placeholders: {
      $1: props.searchQueries.map((query, i, a) => (
        <>
          "<a className={styles.searchQueryLink} href='#' onClick={(e) => handleOpenSearchQuery(e, query)}>
            {query}
          </a>"{(i < a.length-1) ? ', ' : null}
        </>
      ))
    }
  })

  return (
    <div className={styles.searchSummary}>
      <Icon name="brave-icon-search-color" />
      <span>
        {message} <a className={styles.searchLearnMoreLink} href='#' onClick={handleLearnMore}>{getLocale('learnMore')}</a>
      </span>
    </div>
  )
}

export default function AssistantResponse(props: { entry: mojom.ConversationTurn, isEntryInProgress: boolean }) {
  const searchQueriesEvent = props.entry.events?.find(event => !!event.searchQueriesEvent)?.searchQueriesEvent
  const hasCompletionStarted = !props.isEntryInProgress || props.entry.events?.find(event => !!event.completionEvent)

  return (<>
  {
    props.entry.events?.map((event) => {
      if (event.completionEvent) {
        return (
          <MarkdownRenderer
            shouldShowTextCursor={props.isEntryInProgress}
            text={event.completionEvent.completion}
          />
        )
      }
      if (event.searchStatusEvent && props.isEntryInProgress && !hasCompletionStarted) {
        return (
          <div className={styles.actionInProgress}><ProgressRing />Improving answer with Brave Search…</div>
        )
      }
      if (event.pageContentRefineEvent && props.isEntryInProgress && !hasCompletionStarted) {
        return (
          <div className={styles.actionInProgress}><ProgressRing />{getLocale('pageContentRefinedInProgress')}</div>
        )
      }
      // TODO(petemill): Consider displaying in-progress queries if the API
      // timing improves (or worsens for the completion events).
      // if (event.searchQueriesEvent && props.isEntryInProgress) {
      //   return (<>
      //     {event.searchQueriesEvent.searchQueries.map(query => <div className={styles.searchQuery}>Searching for <span className={styles.searchLink}><Icon name="brave-icon-search-color" /><Link href='#'>{query}</Link></span></div>)}
      //   </>)
      // }

      // Unknown events should be ignored
      return null
    })
  }
  { !props.isEntryInProgress && searchQueriesEvent &&
    <SearchSummary searchQueries={searchQueriesEvent.searchQueries} />
  }
  </>)
}
