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

function DeepResearchFinalAnswer(props: {
  answerEvent: Mojom.DeepResearchAnswerEvent
  isEntryInProgress: boolean
}) {
  const { answerEvent, isEntryInProgress } = props

  const citations = answerEvent.citations || []
  const citationUrls = citations.map(c => c.url)

  // Build a map of citation number to URL for inline replacement
  const citationMap = new Map<number, string>()
  citations.forEach(c => citationMap.set(c.number, c.url))

  // Replace [N] markers with clickable markdown links
  let formattedText = answerEvent.text
  // Add space before citation markers
  formattedText = formattedText.replace(/(\w|\S)\[(\d+)\]/g, '$1 [$2]')
  // Convert [N] to markdown links
  formattedText = formattedText.replace(/\[(\d+)\]/g, (match, num) => {
    const url = citationMap.get(parseInt(num, 10))
    if (url) {
      return `[[${num}]](${url})`
    }
    return match
  })

  // Add sources section at the end
  const sourcesSection = citations.length > 0
    ? '\n\n---\n\n**Sources:**\n\n' + citations
        .map((c) => `- [${c.number}] [${new URL(c.url).hostname}](${c.url})`)
        .join('\n')
    : ''

  // Add separator before the report to distinguish from LLM's intro message
  const fullText = `---\n\n${formattedText}${sourcesSection}`

  return (
    <MarkdownRenderer
      shouldShowTextCursor={isEntryInProgress}
      text={fullText}
      allowedLinks={citationUrls}
      disableLinkRestrictions={false}
    />
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
    const completion = event.completionEvent.completion

    // Check if this is a deep research completion (has our Sources section marker)
    // For deep research content, we allow all links since they're verified sources
    const isDeepResearchCompletion = completion.includes('**Sources:**\n\n-')

    const numberedLinks =
      allowedLinks.length > 0
        ? allowedLinks
            .map((url: string, index: number) => `[${index + 1}]: ${url}`)
            .join('\n') + '\n\n'
        : ''

    // Remove citations with missing links (skip for deep research which handles its own citations)
    const filteredCompletion = isDeepResearchCompletion
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
        disableLinkRestrictions={!isLeoModel || isDeepResearchCompletion}
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

  // Extract deep research events - use findLast to get most recent queries
  const deepResearchQueriesEvent = props.events?.findLast(
    (event) => event.deepResearchQueriesEvent,
  )?.deepResearchQueriesEvent
  const deepResearchThinkingEvents = props.events
    ?.filter((event) => event.deepResearchThinkingEvent)
    .map((event) => event.deepResearchThinkingEvent!) ?? []
  // Use findLast to get the final answer (last answer event has final=true)
  const deepResearchAnswerEvent = props.events?.findLast(
    (event) => event.deepResearchAnswerEvent,
  )?.deepResearchAnswerEvent
  // Use findLast for events that update over time to get the most recent
  const deepResearchProgressEvent = props.events?.findLast(
    (event) => event.deepResearchProgressEvent,
  )?.deepResearchProgressEvent
  const deepResearchCompleteEvent = props.events?.find(
    (event) => event.deepResearchCompleteEvent,
  )?.deepResearchCompleteEvent
  const deepResearchErrorEvent = props.events?.find(
    (event) => event.deepResearchErrorEvent,
  )?.deepResearchErrorEvent
  // Granular progress events - use findLast to get most recent
  const deepResearchSearchStatusEvent = props.events?.findLast(
    (event) => event.deepResearchSearchStatusEvent,
  )?.deepResearchSearchStatusEvent
  const deepResearchAnalysisStatusEvent = props.events?.findLast(
    (event) => event.deepResearchAnalysisStatusEvent,
  )?.deepResearchAnalysisStatusEvent
  const deepResearchIterationCompleteEvent = props.events?.findLast(
    (event) => event.deepResearchIterationCompleteEvent,
  )?.deepResearchIterationCompleteEvent
  const deepResearchAnalyzingEvent = props.events?.findLast(
    (event) => event.deepResearchAnalyzingEvent,
  )?.deepResearchAnalyzingEvent
  const deepResearchFetchStatusEvent = props.events?.findLast(
    (event) => event.deepResearchFetchStatusEvent,
  )?.deepResearchFetchStatusEvent

  const hasDeepResearchEvents = deepResearchQueriesEvent
    || deepResearchThinkingEvents.length > 0
    || deepResearchAnswerEvent
    || deepResearchProgressEvent
    || deepResearchCompleteEvent
    || deepResearchErrorEvent
    || deepResearchSearchStatusEvent
    || deepResearchAnalysisStatusEvent
    || deepResearchIterationCompleteEvent
    || deepResearchAnalyzingEvent
    || deepResearchFetchStatusEvent

  const hasCompletionStarted =
    !props.isEntryInProgress
    || props.events.some((event) => event.completionEvent)

  // Check if we have a deep research answer (any answer event with text, including streaming)
  const hasDeepResearchAnswer = !!deepResearchAnswerEvent?.text

  // Filter out deep research events from the main event rendering
  // since we render them in a special component.
  // Also filter out completion events when we have a final deep research answer,
  // since the completion event is only for persistence (the answer event handles rendering).
  const nonDeepResearchEvents = props.events?.filter(
    (event) =>
      !event.deepResearchQueriesEvent
      && !event.deepResearchAnalyzingEvent
      && !event.deepResearchThinkingEvent
      && !event.deepResearchAnswerEvent
      && !event.deepResearchProgressEvent
      && !event.deepResearchBlindspotsEvent
      && !event.deepResearchCompleteEvent
      && !event.deepResearchErrorEvent
      && !event.deepResearchSearchStatusEvent
      && !event.deepResearchFetchStatusEvent
      && !event.deepResearchAnalysisStatusEvent
      && !event.deepResearchIterationCompleteEvent
      // Filter out completion events when we have a deep research answer
      // (the completion event is for persistence, the answer event is for rendering)
      && !(hasDeepResearchAnswer && event.completionEvent),
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
          isEntryInProgress={props.isEntryInProgress}
          isEntryInteractivityAllowed={props.isEntryInteractivityAllowed}
          allowedLinks={props.allowedLinks}
          isLeoModel={props.isLeoModel}
        />
      ))}

      {/* Render deep research progress in a special component (hide after final report shown) */}
      {hasDeepResearchEvents && !hasDeepResearchAnswer && (
        <DeepResearchEvent
          queriesEvent={deepResearchQueriesEvent}
          thinkingEvents={deepResearchThinkingEvents}
          progressEvent={deepResearchProgressEvent}
          completeEvent={deepResearchCompleteEvent}
          errorEvent={deepResearchErrorEvent}
          searchStatusEvent={deepResearchSearchStatusEvent}
          analysisStatusEvent={deepResearchAnalysisStatusEvent}
          iterationCompleteEvent={deepResearchIterationCompleteEvent}
          analyzingEvent={deepResearchAnalyzingEvent}
          fetchStatusEvent={deepResearchFetchStatusEvent}
          isActive={props.isEntryInProgress}
        />
      )}

      {/* Render deep research answer (both streaming and final) */}
      {hasDeepResearchAnswer && (
        <DeepResearchFinalAnswer
          answerEvent={deepResearchAnswerEvent!}
          isEntryInProgress={props.isEntryInProgress}
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
