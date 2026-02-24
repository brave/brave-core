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
import Chart from './chart'
import DeepResearchEvent from './deep_research_event'
import { extractDeepResearchEvents } from './deep_research_utils'
import styles from './style.module.scss'
import {
  removeReasoning,
  removeCitationsWithMissingLinks,
  normalizeCitationSpacing,
} from '../conversation_entries/conversation_entries_utils'
import RichSearchWidget from './rich_search_widget'
import AssistantResponseContextProvider from './assistant_response_context'

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
        <button
          className={styles.searchQueryLink}
          onClick={(e) => handleOpenSearchQuery(e, query)}
        >
          {query}
        </button>
        "{i < a.length - 1 ? ', ' : null}
      </React.Fragment>
    )),
  })

  return (
    <div className={styles.searchSummary}>
      <Icon name='brave-icon-search-color' />
      <span data-test-id='search-summary'>
        {message}{' '}
        <button
          className={styles.searchLearnMoreLink}
          onClick={handleLearnMore}
        >
          {getLocale(S.CHAT_UI_LEARN_MORE)}
        </button>
      </span>
    </div>
  )
}

function AssistantEvent(
  props: BaseProps & {
    event: Mojom.ConversationEntryEvent
    hasCompletionStarted: boolean
    isDeepResearchResponse: boolean
  },
) {
  const { allowedLinks, event, isEntryInProgress, isLeoModel } = props
  const context = useUntrustedConversationContext()

  if (event.completionEvent) {
    const completion = event.completionEvent.completion

    // Deep research uses inline URL citations (e.g. [Title](url)) rather
    // than numbered reference-style links, so skip both the numbered prefix
    // and the citation filtering that are designed for the [1], [2] pattern.
    const numberedLinks =
      allowedLinks.length > 0 && !props.isDeepResearchResponse
        ? allowedLinks
            .map((url: string, index: number) => `[${index + 1}]: ${url}`)
            .join('\n') + '\n\n'
        : ''

    const filteredCompletion = props.isDeepResearchResponse
      ? completion
      : removeCitationsWithMissingLinks(completion, allowedLinks)

    const processedCompletion = normalizeCitationSpacing(
      filteredCompletion,
    )

    const fullText = `${numberedLinks}${removeReasoning(processedCompletion)}`

    return (
      <MarkdownRenderer
        shouldShowTextCursor={isEntryInProgress}
        text={fullText}
        allowedLinks={allowedLinks}
        // Deep research uses inline URL citations ([Title](url)) rather than
        // numbered references ([1]), so link restrictions must be disabled since
        // the allowedLinks/citation filtering mechanism only handles numbered
        // reference-style links.
        disableLinkRestrictions={!isLeoModel || props.isDeepResearchResponse}
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
  toolArtifacts?: Mojom.ToolArtifact[] | null
}

export default function AssistantResponse(props: AssistantResponseProps) {
  // Aggregate sources/queries across all events since
  // multiple tool results each emit their own event.
  const allSources = props.events.flatMap(
    (event) => event.sourcesEvent?.sources ?? [],
  )
  const allRichResults = props.events.flatMap(
    (event) => event.sourcesEvent?.richResults?.filter((r) => !!r) ?? [],
  )
  const allSearchQueries = props.events.flatMap(
    (event) => event.searchQueriesEvent?.searchQueries ?? [],
  )

  const deepResearch = extractDeepResearchEvents(props.events)

  const hasCompletionStarted =
    !props.isEntryInProgress
    || props.events.some((event) => event.completionEvent)

  // Filter out deep research events from the main event rendering
  // since we render them in a special progress component.
  const nonDeepResearchEvents = props.events?.filter(
    (event) => !event.deepResearchEvent,
  )

  return (
    <AssistantResponseContextProvider events={props.events}>
      {allRichResults.map((r) => (
        <RichSearchWidget
          key={r}
          jsonData={r}
        />
      ))}

      {/* Render LLM's initial response first (e.g., "I'll conduct research...") */}
      {nonDeepResearchEvents?.map((event, i) => (
        <AssistantEvent
          key={i}
          event={event}
          hasCompletionStarted={hasCompletionStarted}
          isDeepResearchResponse={deepResearch.hasDeepResearchEvents}
          isEntryInProgress={props.isEntryInProgress}
          isEntryInteractivityAllowed={props.isEntryInteractivityAllowed}
          allowedLinks={props.allowedLinks}
          isLeoModel={props.isLeoModel}
        />
      ))}

      {/* Render deep research progress while research is active.
          Hide once research completes - the final answer streams in
          as standard completion events rendered above. */}
      {deepResearch.hasDeepResearchEvents && props.isEntryInProgress
        && !deepResearch.completeEvent && (
        <DeepResearchEvent
          queriesEvent={deepResearch.queriesEvent}
          thinkingEvents={deepResearch.thinkingEvents}
          progressEvent={deepResearch.progressEvent}
          completeEvent={deepResearch.completeEvent}
          errorEvent={deepResearch.errorEvent}
          searchStatusEvent={deepResearch.searchStatusEvent}
          analysisStatusEvent={deepResearch.analysisStatusEvent}
          iterationCompleteEvent={deepResearch.iterationCompleteEvent}
          analyzingEvent={deepResearch.analyzingEvent}
          fetchStatusEvent={deepResearch.fetchStatusEvent}
          isActive={props.isEntryInProgress}
        />
      )}

      {!props.isEntryInProgress && allSources.length > 0 && (
        <WebSourcesEvent sources={allSources} />
      )}
      {props.toolArtifacts
        ?.filter((artifact) => artifact.type === Mojom.LINE_CHART_ARTIFACT_TYPE)
        .map((artifact, i) => (
          <Chart
            key={i}
            artifact={artifact}
          />
        ))}
      {!props.isEntryInProgress && allSearchQueries.length > 0 && (
        <SearchSummary searchQueries={allSearchQueries} />
      )}
    </AssistantResponseContextProvider>
  )
}
