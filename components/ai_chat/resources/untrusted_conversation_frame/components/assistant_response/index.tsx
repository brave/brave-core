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
import ToolEvent from './tool_event'
import WebSourcesEvent from './web_sources_event'
import MemoryToolEvent from './memory_tool_event'
import styles from './style.module.scss'
import {
  removeReasoning,
  removeCitationsWithMissingLinks,
} from '../conversation_entries/conversation_entries_utils'
import RichSearchWidget from './rich_search_widget'

interface BaseProps {
  // Whether data is currently being received (generated)
  isEntryInProgress: boolean
  // Whether it's possible to interact with the entry's tool use requests
  isEntryInteractivityAllowed: boolean
  // Only these urls should be rendered as links
  allowedLinks: string[]
  isLeoModel: boolean
}

function SearchSummary(props: { searchQueries: string[] }) {
  const context = useUntrustedConversationContext()

  const handleOpenSearchQuery = React.useCallback(
    (e: React.MouseEvent, query: string) => {
      e.preventDefault()
      context.uiHandler?.openSearchURL(query)
    },
    [],
  )

  const handleLearnMore = () => {
    context.uiHandler?.openLearnMoreAboutBraveSearchWithLeo()
  }

  const message = formatLocale(S.CHAT_UI_SEARCH_QUERIES, {
    $1: props.searchQueries.map((query, i, a) => (
      <React.Fragment key={i}>
        "
        <a
          className={styles.searchQueryLink}
          href='#'
          onClick={(e) => handleOpenSearchQuery(e, query)}
        >
          {query}
        </a>
        "{i < a.length - 1 ? ', ' : null}
      </React.Fragment>
    )),
  })

  return (
    <div className={styles.searchSummary}>
      <Icon name='brave-icon-search-color' />
      <span>
        {message}{' '}
        <a
          className={styles.searchLearnMoreLink}
          href='#'
          onClick={handleLearnMore}
        >
          {getLocale(S.CHAT_UI_LEARN_MORE)}
        </a>
      </span>
    </div>
  )
}

const AssistantEvent = React.memo(
  function AssistantEvent(
    props: BaseProps & {
      event: Mojom.ConversationEntryEvent
      hasCompletionStarted: boolean
      eventIndex: number
    },
  ) {
    const { allowedLinks, event, isEntryInProgress, isLeoModel, eventIndex } =
      props

    // Store previous text content for each element to track new text chunks.
    // Key: element index, Value: previous text content
    // This ref persists for the lifetime of this AssistantEvent component
    // instance.
    const elementTextStateRef = React.useRef<Map<number, string>>(new Map())

    // Memoize text processing to avoid recalculating on every render
    const fullText = React.useMemo(() => {
      if (!event.completionEvent) {
        return ''
      }

      const numberedLinks =
        allowedLinks.length > 0
          ? allowedLinks
              .map((url: string, index: number) => `[${index + 1}]: ${url}`)
              .join('\n') + '\n\n'
          : ''

      // Remove citations with missing links
      const filteredOutCitationsWithMissingLinks =
        removeCitationsWithMissingLinks(
          event.completionEvent.completion,
          allowedLinks,
        )

      // Replaces 2 consecutive citations with a separator and also
      // adds a space before the citation and the text.
      const completion = filteredOutCitationsWithMissingLinks.replace(
        /(\w|\S)\[(\d+)\]/g,
        '$1 [$2]',
      )

      return `${numberedLinks}${removeReasoning(completion)}`
    }, [event.completionEvent?.completion, allowedLinks])

    if (event.completionEvent) {
      // Use a stable key to ensure MarkdownRenderer is only mounted once
      // and persists across updates, only receiving text prop updates
      const stableKey = getEventKey(event, eventIndex)
      return (
        <MarkdownRenderer
          key={`markdown-${stableKey}`}
          isEntryInProgress={isEntryInProgress}
          text={fullText}
          allowedLinks={allowedLinks}
          disableLinkRestrictions={!isLeoModel}
          elementTextStateRef={elementTextStateRef}
        />
      )
    }
    if (
      props.event.searchStatusEvent
      && props.isEntryInProgress
      && !props.hasCompletionStarted
    ) {
      return (
        <div className={styles.actionInProgress}>
          <ProgressRing />
          Improving answer with Brave Search…
        </div>
      )
    }
    if (props.event.toolUseEvent) {
      if (
        props.event.toolUseEvent.toolName === Mojom.MEMORY_STORAGE_TOOL_NAME
      ) {
        return <MemoryToolEvent toolUseEvent={props.event.toolUseEvent} />
      }
      return (
        <ToolEvent
          toolUseEvent={props.event.toolUseEvent}
          isEntryActive={props.isEntryInteractivityAllowed}
        />
      )
    }

    // Unknown events should be ignored
    return null
  },
  (prevProps, nextProps) => {
    // Returns true if props are equal (skip re-render)
    // For completion events, check if completion text changed
    if (prevProps.event.completionEvent && nextProps.event.completionEvent) {
      const completionChanged =
        prevProps.event.completionEvent.completion
        !== nextProps.event.completionEvent.completion
      const otherPropsChanged =
        prevProps.isEntryInProgress !== nextProps.isEntryInProgress
        || prevProps.isEntryInteractivityAllowed
          !== nextProps.isEntryInteractivityAllowed
        || prevProps.hasCompletionStarted !== nextProps.hasCompletionStarted
        || prevProps.isLeoModel !== nextProps.isLeoModel
        || JSON.stringify(prevProps.allowedLinks)
          !== JSON.stringify(nextProps.allowedLinks)

      // Skip re-render only if nothing changed
      return !completionChanged && !otherPropsChanged
    }

    // For other event types, check if event reference or other props changed
    const eventChanged = prevProps.event !== nextProps.event
    const otherPropsChanged =
      prevProps.isEntryInProgress !== nextProps.isEntryInProgress
      || prevProps.isEntryInteractivityAllowed
        !== nextProps.isEntryInteractivityAllowed
      || prevProps.hasCompletionStarted !== nextProps.hasCompletionStarted
      || prevProps.isLeoModel !== nextProps.isLeoModel
      || JSON.stringify(prevProps.allowedLinks)
        !== JSON.stringify(nextProps.allowedLinks)

    // Skip re-render only if nothing changed
    return !eventChanged && !otherPropsChanged
  },
)

export type AssistantResponseProps = BaseProps & {
  events: Mojom.ConversationEntryEvent[]
}

// Generate a stable key for an event based on its type and content
function getEventKey(
  event: Mojom.ConversationEntryEvent,
  index: number,
): string {
  if (event.completionEvent) {
    // For completion events, use a combination of type and index
    // The index helps distinguish multiple completion events
    return `completion-${index}`
  }
  if (event.toolUseEvent) {
    // For tool events, use tool name and index for uniqueness
    return `tool-${event.toolUseEvent.toolName}-${index}`
  }
  if (event.searchStatusEvent) {
    return `search-status-${index}`
  }
  if (event.searchQueriesEvent) {
    return `search-queries-${index}`
  }
  if (event.sourcesEvent) {
    return `sources-${index}`
  }
  // Fallback to index for unknown event types
  return `event-${index}`
}

export default function AssistantResponse(props: AssistantResponseProps) {
  // Extract certain events which need to render at specific locations (e.g. end of the events)
  const searchQueriesEvent = props.events?.find(
    (event) => event.searchQueriesEvent,
  )?.searchQueriesEvent
  const sourcesEvent = props.events?.find(
    (event) => !!event.sourcesEvent,
  )?.sourcesEvent

  const hasCompletionStarted =
    !props.isEntryInProgress
    || (props.events?.some((event) => event.completionEvent) ?? false)

  return (
    <>
      {sourcesEvent?.richResults
        .filter((r) => r)
        .map((r) => (
          <RichSearchWidget
            key={r}
            jsonData={r}
          />
        ))}
      {props.events?.map((event, i) => (
        <AssistantEvent
          key={getEventKey(event, i)}
          event={event}
          eventIndex={i}
          hasCompletionStarted={hasCompletionStarted}
          isEntryInProgress={props.isEntryInProgress}
          isEntryInteractivityAllowed={props.isEntryInteractivityAllowed}
          allowedLinks={props.allowedLinks}
          isLeoModel={props.isLeoModel}
        />
      ))}

      {!props.isEntryInProgress && (
        <>
          {sourcesEvent && <WebSourcesEvent sources={sourcesEvent.sources} />}
          {searchQueriesEvent && (
            <SearchSummary searchQueries={searchQueriesEvent.searchQueries} />
          )}
        </>
      )}
    </>
  )
}
