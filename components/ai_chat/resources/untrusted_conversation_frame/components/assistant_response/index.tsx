// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'
import Tooltip from '@brave/leo/react/tooltip'
import formatMessage from '$web-common/formatMessage'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'


import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import MarkdownRenderer from '../markdown_renderer'
import WebSourcesEvent from './web_sources_event'
import ToolEvent, { useToolEventContent } from './tool_event'
import styles from './style.module.scss'
import classnames from '$web-common/classnames'

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
interface AssistantEventProps {
  hasCompletionStarted: boolean,
  isEntryInProgress: boolean,
  isActiveEntry: boolean
}

function AssistantEvent(props: AssistantEventProps & { event: Mojom.ConversationEntryEvent }) {
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
    return <ToolEvent event={props.event.toolUseEvent} isActiveEntry={props.isActiveEntry} />
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

export default function AssistantResponse(props: { events: Mojom.ConversationEntryEvent[], isLastEntry: boolean }) {
  const context = useUntrustedConversationContext()
  // Extract certain events which need to render at specific locations (e.g. end of the events)
  // If the entry is currently being acted on, whether it's still generating or not, the tools may
  // still be evaluating.
  const isEntryInProgress = props.isLastEntry && context.isGenerating
  const searchQueriesEvent = props.events?.find(event => event.searchQueriesEvent)?.searchQueriesEvent
  const sourcesEvent = props.events?.find(event => !!event.sourcesEvent)?.sourcesEvent

  const hasCompletionStarted = !isEntryInProgress ||
    (props.events?.some(event => event.completionEvent) ?? false)

  const isGroupable = props.events?.some(event => event.toolUseEvent?.toolName === 'computer')

  return (<>
  {
    isGroupable ?
    <AssistantEventGroup
      events={props.events}
      hasCompletionStarted={hasCompletionStarted}
      isEntryInProgress={isEntryInProgress}
      isActiveEntry={props.isLastEntry}
    /> : props.events.map((event, i) =>
      <AssistantEvent
        key={i}
        event={event}
        hasCompletionStarted={hasCompletionStarted}
        isActiveEntry={props.isLastEntry}
        isEntryInProgress={isEntryInProgress}
      />
    )
  }

  {
    !isEntryInProgress &&
    <>
      {searchQueriesEvent && <SearchSummary searchQueries={searchQueriesEvent.searchQueries} />}
      {sourcesEvent && <WebSourcesEvent sources={sourcesEvent.sources} />}
    </>
  }
  </>)
}

function AssistantEventGroup(props: AssistantEventProps & { events: Mojom.ConversationEntryEvent[] }) {
  const [isExpanded, setIsExpanded] = React.useState(false)

  const statusText = props.isEntryInProgress
    ? 'Browse with Leo in progress'
    : 'Leo used the browser'

  const expandText = isExpanded ? 'Hide details' : 'Show details'

  let lastCompletionEvent: Mojom.ConversationEntryEvent | null = null
  const eventsRequiringInteraction: Mojom.ConversationEntryEvent[] = []
  const miniEvents: Mojom.ConversationEntryEvent[] = []
  const toolsRequiringInteraction = ['user_choice_tool', 'active_web_page_content_fetcher']

  if (!isExpanded) {
    for (const event of props.events) {
      if (event.completionEvent) {
        lastCompletionEvent = event
        continue
      }
      if (
        toolsRequiringInteraction.includes(event.toolUseEvent?.toolName ?? '') &&
        (!event.toolUseEvent?.output || !event.toolUseEvent?.output.length)) {
        eventsRequiringInteraction.push(event)
        continue
      }
      miniEvents.unshift(event)
    }
  }

  const eventsToDisplay = isExpanded ? props.events : eventsRequiringInteraction
  if (!isExpanded && lastCompletionEvent) {
    eventsToDisplay.unshift(lastCompletionEvent)
  }

  return (
    <>
      <div className={styles.eventGroupHeader}>
        <Icon name='cursor-leo' />
        {statusText}
        <button onClick={() => setIsExpanded(!isExpanded)}>{expandText}</button>
      </div>
      {!!miniEvents.length &&
        <div className={styles.miniEvents}>
          {miniEvents.map((event, i) =>
            <MiniEvent
              key={miniEvents.length - 1 - i}
              isNew={props.isEntryInProgress && i === 0}
              event={event}
            />
          )}
        </div>
      }
      {eventsToDisplay.map((event, i) =>
        <AssistantEvent
          key={i}
          event={event}
          hasCompletionStarted={props.hasCompletionStarted}
          isActiveEntry={props.isActiveEntry && i === eventsToDisplay.length - 1}
          isEntryInProgress={props.isEntryInProgress && i === eventsToDisplay.length - 1}
        />
      )}
    </>
  )
}

function MiniEvent(props: { event: Mojom.ConversationEntryEvent, isNew: boolean }) {
  if (!props.event.toolUseEvent) {
    return null
  }

  const toolEventContent = useToolEventContent(props.event.toolUseEvent)

  if (!toolEventContent.iconName) {
    return null
  }

  return (
    <Tooltip mouseleaveTimeout={20}>
      <Icon
        className={classnames(styles.miniEventIcon, props.isNew && styles.isNew)}
        name={toolEventContent.iconName}
      />
      <div slot='content'>
        <div className={styles.toolUseContentPreview}>
          {toolEventContent.toolText}
        </div>
        {toolEventContent.tooltipContent}
      </div>
    </Tooltip>
  )
}
