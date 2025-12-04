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
import DeepResearchPanel from '../deep_research_panel'
import { convertMojomEvent } from '../deep_research_panel/deep_research_types'
import type { DeepResearchMessage } from '../deep_research_panel/deep_research_types'
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

function AssistantEvent(
  props: BaseProps & {
    event: Mojom.ConversationEntryEvent
    hasCompletionStarted: boolean
  },
) {
  const { allowedLinks, event, isEntryInProgress, isLeoModel } = props
  const context = useUntrustedConversationContext()

  if (event.completionEvent) {
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

    const fullText = `${numberedLinks}${removeReasoning(completion)}`

    return (
      <MarkdownRenderer
        shouldShowTextCursor={isEntryInProgress}
        text={fullText}
        allowedLinks={allowedLinks}
        disableLinkRestrictions={!isLeoModel}
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
    if (props.event.toolUseEvent.toolName === Mojom.MEMORY_STORAGE_TOOL_NAME) {
      return <MemoryToolEvent toolUseEvent={props.event.toolUseEvent} />
    }
    return (
      <ToolEvent
        toolUseEvent={props.event.toolUseEvent}
        isEntryActive={props.isEntryInteractivityAllowed}
        isExecuting={context.isToolExecuting}
      />
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

export type AssistantResponseProps = BaseProps & {
  events: Mojom.ConversationEntryEvent[]
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

  // Detect and accumulate deep research events
  // Separate research events from completion to avoid reprocessing on every token
  const researchEventsOnly = React.useMemo(() => {
    return props.events?.filter(event =>
      event.thinkingEvent || event.blindspotsEvent || event.progressEvent ||
      event.searchQueriesEvent || event.searchStatusEvent ||
      event.imageResultsEvent || event.newsResultsEvent || event.discussionResultsEvent
    ) ?? []
  }, [props.events?.length]) // Only recompute when events array length changes

  const deepResearchMessage = React.useMemo((): DeepResearchMessage | null => {
    if (researchEventsOnly.length === 0) return null

    const researchEvents = researchEventsOnly
      .map(event => convertMojomEvent(event))
      .filter((event): event is NonNullable<typeof event> => event !== null)

    return {
      events: researchEvents,
      finished: !props.isEntryInProgress
    }
  }, [researchEventsOnly, props.isEntryInProgress])

  // Create a Set of research event indices for O(1) lookup
  const researchEventIndices = React.useMemo(() => {
    if (!deepResearchMessage) return null
    const indices = new Set<number>()
    props.events?.forEach((event, i) => {
      if (event.thinkingEvent || event.blindspotsEvent ||
        event.progressEvent || event.searchQueriesEvent ||
        event.searchStatusEvent || event.imageResultsEvent ||
        event.newsResultsEvent || event.discussionResultsEvent) {
        indices.add(i)
      }
    })
    return indices
  }, [deepResearchMessage, props.events?.length])

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
      {/* Show deep research panel if research events exist and not finished */}
      {deepResearchMessage && !deepResearchMessage.finished && (
        <DeepResearchPanel message={deepResearchMessage} />
      )}
      {/* Always render events, but filter out research-specific ones if panel is shown */}
      {props.events?.map((event, i) => {
        // Skip research events that are shown in the panel - O(1) lookup
        if (researchEventIndices?.has(i)) {
          return null
        }

        return (
          <AssistantEvent
            key={i}
            event={event}
            hasCompletionStarted={hasCompletionStarted}
            isEntryInProgress={props.isEntryInProgress}
            isEntryInteractivityAllowed={props.isEntryInteractivityAllowed}
            allowedLinks={props.allowedLinks}
            isLeoModel={props.isLeoModel}
          />
        )
      })}

      {!props.isEntryInProgress && !deepResearchMessage && (
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
