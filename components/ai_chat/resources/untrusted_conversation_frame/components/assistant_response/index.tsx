// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import ProgressRing from '@brave/leo/react/progressRing'
import Icon from '@brave/leo/react/icon'
import formatMessage from '$web-common/formatMessage'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import MarkdownRenderer from '../markdown_renderer'
import styles from './style.module.scss'
import Button from '@brave/leo/react/button'

function SearchSummary (props: { searchQueries: string[] }) {
  const context = useUntrustedConversationContext()

  const handleOpenSearchQuery = React.useCallback((e: React.MouseEvent, query: string) => {
    e.preventDefault()
    context.uiHandler?.openSearchURL(query)
  }, [])

  const handleLearnMore = () => {
    context.uiHandler?.openLearnMoreAboutBraveSearchWithLeo()
  }

  const message = formatMessage(getLocale('searchQueries'), {
    placeholders: {
      $1: props.searchQueries.map((query, i, a) => (
        <React.Fragment key={i}>
          "<a className={styles.searchQueryLink} href='#' onClick={(e) => handleOpenSearchQuery(e, query)}>
            {query}
          </a>"{(i < a.length-1) ? ', ' : null}
        </React.Fragment>
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

function AssistantEvent(props: { event: Mojom.ConversationEntryEvent, hasCompletionStarted: boolean, isEntryInProgress: boolean }) {
  const context = useUntrustedConversationContext()

  if (props.event.completionEvent) {
    return (
      <MarkdownRenderer
        shouldShowTextCursor={props.isEntryInProgress}
        text={props.event.completionEvent.completion}
      />
    )
  }
  if (props.event.searchStatusEvent && props.isEntryInProgress && !props.hasCompletionStarted) {
    return (
      <div className={styles.actionInProgress}><ProgressRing />Improving answer with Brave Search…</div>
    )
  }
  if (props.event.pageContentRefineEvent && props.isEntryInProgress && !props.hasCompletionStarted) {
    return (
      <div className={styles.actionInProgress}><ProgressRing />{getLocale('pageContentRefinedInProgress')}</div>
    )
  }
  else if (props.event.toolUseEvent) {
    const toolUse = props.event.toolUseEvent
    if (toolUse.toolName === 'active_web_page_content_fetcher') {
      return (
        <div className={styles.toolUseRequest}>Leo is requesting access to page content
          <Button onClick={() => context.conversationHandler?.respondToToolUseRequest(toolUse.toolId, null)}>Allow</Button>
        </div>
      )
    }
    return (
      <div>**{toolUse.toolName} - {toolUse.inputJson}</div>
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
}

export default function AssistantResponse(props: { entry: Mojom.ConversationTurn, isEntryInProgress: boolean }) {
  const searchQueriesEvent = props.entry.events?.find(event => event.searchQueriesEvent)?.searchQueriesEvent
  const hasCompletionStarted = !props.isEntryInProgress ||
    (props.entry.events?.some(event => event.completionEvent) ?? false)

  return (<>
  {
    props.entry.events?.map((event, i) =>
      <AssistantEvent
        key={i}
        event={event}
        hasCompletionStarted={hasCompletionStarted}
        isEntryInProgress={props.isEntryInProgress}
      />
    )
  }
  { !props.isEntryInProgress && searchQueriesEvent &&
    <SearchSummary searchQueries={searchQueriesEvent.searchQueries} />
  }
  </>)
}
