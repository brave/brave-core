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
import GeneratedFilterDisplay from '../generated_filter_display'
import styles from './style.module.scss'
import {
  removeReasoning,
  removeCitationsWithMissingLinks,
} from '../conversation_entries/conversation_entries_utils'

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
    hasFilterGenerationTool?: boolean
  },
) {
  const { allowedLinks, event, isEntryInProgress, isLeoModel, hasFilterGenerationTool } = props

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
    let completion = filteredOutCitationsWithMissingLinks.replace(
      /(\w|\S)\[(\d+)\]/g,
      '$1 [$2]',
    )

    // If this is a text_filter_generation response, strip out only the scriptlet name
    // and filter rule (shown in the button UI dropdown), but keep the JavaScript code visible
    if (hasFilterGenerationTool) {
      // Remove scriptlet name section - match with or without bold markdown
      completion = completion.replace(/(\*\*)?Scriptlet name:(\*\*)?[\s\S]*?```[\s\S]*?```/gi, '')
      // Remove filter rule section - match with or without bold markdown
      completion = completion.replace(/(\*\*)?Filter rule:(\*\*)?[\s\S]*?```[\s\S]*?```/gi, '')
      // Clean up multiple consecutive blank lines
      completion = completion.replace(/\n\s*\n\s*\n/g, '\n\n')
      // Clean up extra whitespace
      completion = completion.trim()
    }

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
    console.log('[AssistantEvent] Found toolUseEvent', {
      toolName: props.event.toolUseEvent.toolName,
      hasOutput: !!props.event.toolUseEvent.output,
      outputLength: props.event.toolUseEvent.output?.length,
    })
    if (props.event.toolUseEvent.toolName === Mojom.MEMORY_STORAGE_TOOL_NAME) {
      console.log('[AssistantEvent] Rendering MemoryToolEvent')
      return <MemoryToolEvent toolUseEvent={props.event.toolUseEvent} />
    }
    console.log('[AssistantEvent] Rendering ToolEvent for:', props.event.toolUseEvent.toolName)
    return (
      <ToolEvent
        toolUseEvent={props.event.toolUseEvent}
        isEntryActive={props.isEntryInteractivityAllowed}
      />
    )
  }
  if (props.event.generatedFilterEvent) {
    const filterEvent = props.event.generatedFilterEvent
    return (
      <GeneratedFilterDisplay
        filter={{
          filterType: filterEvent.filterType === Mojom.GeneratedFilterType.CSS_SELECTOR
            ? 'css_selector'
            : 'scriptlet',
          domain: filterEvent.domain,
          code: filterEvent.code,
          description: filterEvent.description,
          targetElements: filterEvent.targetElements,
          confidence: filterEvent.confidence as 'high' | 'medium' | 'low',
          reasoning: filterEvent.reasoning,
        }}
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

  return (
    <>
      {props.events?.map((event, i) => {
        // Check if this entry has a text_filter_generation tool event
        const hasFilterGenerationTool = props.events?.some(
          e => e.toolUseEvent?.toolName === 'text_filter_generation'
        )

        return (
          <AssistantEvent
            key={i}
            event={event}
            hasCompletionStarted={hasCompletionStarted}
            isEntryInProgress={props.isEntryInProgress}
            isEntryInteractivityAllowed={props.isEntryInteractivityAllowed}
            allowedLinks={props.allowedLinks}
            isLeoModel={props.isLeoModel}
            hasFilterGenerationTool={hasFilterGenerationTool}
          />
        )
      })}

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
