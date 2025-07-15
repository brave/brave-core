// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import ProgressRing from '@brave/leo/react/progressRing'
import Icon from '@brave/leo/react/icon'
import { getLocale, formatLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import MarkdownRenderer from '../markdown_renderer'
import WebSourcesEvent from './web_sources_event'
import styles from './style.module.scss'
import {
  removeReasoning,
  removeCitationsWithMissingLinks
} from '../conversation_entries/conversation_entries_utils'

function SearchSummary (props: { searchQueries: string[] }) {
  const context = useUntrustedConversationContext()

  const handleOpenSearchQuery = React.useCallback((e: React.MouseEvent, query: string) => {
    e.preventDefault()
    context.uiHandler?.openSearchURL(query)
  }, [])

  const handleLearnMore = () => {
    context.uiHandler?.openLearnMoreAboutBraveSearchWithLeo()
  }

  const message = formatLocale(S.CHAT_UI_SEARCH_QUERIES, {
      $1: props.searchQueries.map((query, i, a) => (
        <React.Fragment key={i}>
          "<a className={styles.searchQueryLink} href='#' onClick={(e) => handleOpenSearchQuery(e, query)}>
            {query}
          </a>"{(i < a.length-1) ? ', ' : null}
        </React.Fragment>
      ))
  })

  return (
    <div className={styles.searchSummary}>
      <Icon name="brave-icon-search-color" />
      <span>
        {message} <a className={styles.searchLearnMoreLink} href='#' onClick={handleLearnMore}>{getLocale(S.CHAT_UI_LEARN_MORE)}</a>
      </span>
    </div>
  )
}

function AssistantEvent(props: {
  event: Mojom.ConversationEntryEvent,
  hasCompletionStarted: boolean,
  isEntryInProgress: boolean,
  allowedLinks: string[],
  isLeoModel: boolean
}) {
  const { allowedLinks, event, isEntryInProgress, isLeoModel } = props;

  if (event.completionEvent) {
    const numberedLinks =
      allowedLinks.length > 0
        ? allowedLinks.map((url: string, index: number) =>
                           `[${index + 1}]: ${url}`).join('\n') + '\n\n'
        : '';

    // Remove citations with missing links
    const filteredOutCitationsWithMissingLinks =
      removeCitationsWithMissingLinks(
        event.completionEvent.completion,
        allowedLinks
      )

    // Replaces 2 consecutive citations with a separator and also
    // adds a space before the citation and the text.
    const completion =
      filteredOutCitationsWithMissingLinks
        .replace(/(\w|\S)\[(\d+)\]/g, '$1 [$2]')

    const fullText = `${numberedLinks}${removeReasoning(completion)}`;

    return (
      <MarkdownRenderer
        shouldShowTextCursor={isEntryInProgress}
        text={fullText}
        allowedLinks={allowedLinks}
        disableLinkRestrictions={!isLeoModel}
      />
    )
  }
  if (props.event.searchStatusEvent && props.isEntryInProgress && !props.hasCompletionStarted) {
    return (
      <div className={styles.actionInProgress}><ProgressRing />Improving answer with Brave Search…</div>
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

export default function AssistantResponse(props: {
  entry: Mojom.ConversationTurn,
  isEntryInProgress: boolean,
  allowedLinks: string[],
  isLeoModel: boolean
}) {
  // Extract certain events which need to render at specific locations (e.g. end of the events)
  const searchQueriesEvent = props.entry.events?.find(event => event.searchQueriesEvent)?.searchQueriesEvent
  const sourcesEvent = props.entry.events?.find(event => !!event.sourcesEvent)?.sourcesEvent

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
        allowedLinks={props.allowedLinks}
        isLeoModel={props.isLeoModel}
      />
    )
  }

  {
    !props.isEntryInProgress &&
    <>
      {sourcesEvent && <WebSourcesEvent sources={sourcesEvent.sources} />}
      {searchQueriesEvent &&
        <SearchSummary searchQueries={searchQueriesEvent.searchQueries} />
      }
    </>
  }
  </>)
}
