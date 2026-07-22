// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
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
  findTaskCheckboxBracketOffsets,
  normalizeCitationSpacing,
  removeCitationsWithMissingLinks,
  removeReasoning,
} from '../conversation_entries/conversation_entries_utils'
import RichSearchWidget from './rich_search_widget'
import AssistantResponseContextProvider from './assistant_response_context'

interface BaseProps {
  // Whether data is currently being received (generated)
  isEntryInProgress: boolean
  // Whether it's possible to interact with the entry's tool use requests
  isEntryInteractivityAllowed: boolean
  // Citation URLs, in citation order, used to expand `[n]` references.
  allowedLinks: string[]
}

function AssistantEvent(
  props: BaseProps & {
    event: Mojom.ConversationEntryEvent
    hasCompletionStarted: boolean
    entryUuid?: string
  },
) {
  const { allowedLinks, event, isEntryInProgress, entryUuid } = props
  const context = useUntrustedConversationContext()

  if (event.completionEvent) {
    const completion = event.completionEvent.completion
    const numberedLinks =
      allowedLinks.length > 0
        ? allowedLinks
            .map((url: string, index: number) => `[${index + 1}]: ${url}`)
            .join('\n') + '\n\n'
        : ''

    const filteredCompletion = removeCitationsWithMissingLinks(
      completion,
      allowedLinks,
    )

    const processedCompletion = normalizeCitationSpacing(filteredCompletion)

    const fullText = `${numberedLinks}${removeReasoning(processedCompletion)}`

    // Persist a checkbox toggle back to the conversation. The renderer
    // identifies the checkbox by its position among GFM task items in
    // document order, which is invariant to the numbered-links prefix
    // and to reasoning being stripped. We save `processedCompletion`
    // (reasoning preserved, no numbered-links block) — citations that
    // were dropped because their link is missing stay dropped, and
    // citation spacing stays normalized.
    const onToggleCheckbox =
      entryUuid && !isEntryInProgress
        ? (index: number, checked: boolean) => {
            const offsets = findTaskCheckboxBracketOffsets(processedCompletion)
            const at = offsets[index]
            if (at === undefined) return
            const next =
              processedCompletion.slice(0, at + 1)
              + (checked ? 'x' : ' ')
              + processedCompletion.slice(at + 2)
            if (next === processedCompletion) return
            context.conversationHandler?.modifyConversation(
              entryUuid,
              next,
              null,
            )
          }
        : undefined

    return (
      <MarkdownRenderer
        shouldShowTextCursor={isEntryInProgress}
        text={fullText}
        allowedLinks={allowedLinks}
        onToggleCheckbox={onToggleCheckbox}
      />
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

  // Unknown events should be ignored
  return null
}

export type AssistantResponseProps = BaseProps & {
  events: Mojom.ConversationEntryEvent[]
  toolArtifacts?: Mojom.ToolArtifact[] | null
  // The UUID of the original conversation turn this response renders.
  // Required to persist edits (e.g. checkbox toggles) back via
  // ModifyConversation. When omitted, checkbox toggles are inert.
  entryUuid?: string
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

  const deepResearch = React.useMemo(
    () => extractDeepResearchEvents(props.events),
    [props.events],
  )

  const hasCompletionStarted =
    !props.isEntryInProgress
    || props.events.some((event) => event.completionEvent)

  return (
    <AssistantResponseContextProvider events={props.events}>
      <div className={styles.assistantResponse}>
        {allRichResults.map((r) => (
          <RichSearchWidget
            key={r}
            jsonData={r}
          />
        ))}

        {props.events?.map((event, i) => (
          <AssistantEvent
            key={i}
            event={event}
            hasCompletionStarted={hasCompletionStarted}
            isEntryInProgress={props.isEntryInProgress}
            isEntryInteractivityAllowed={props.isEntryInteractivityAllowed}
            allowedLinks={props.allowedLinks}
            entryUuid={props.entryUuid}
          />
        ))}

        {/* Render deep research progress while research is active.
          Hide once the synthesis answer starts streaming in. Keep visible
          during the synthesis phase (after completeEvent but before the
          final completionEvent arrives), ignoring any pre-tool completion
          text the LLM may have emitted before calling deep_research. */}
        {deepResearch.hasDeepResearchEvents
          && props.isEntryInProgress
          && !deepResearch.hasSynthesisCompletion && (
            <DeepResearchEvent
              deepResearch={deepResearch}
              isActive={props.isEntryInProgress}
            />
          )}

        {!props.isEntryInProgress && allSources.length > 0 && (
          <WebSourcesEvent sources={allSources} />
        )}
        {props.toolArtifacts
          ?.filter(
            (artifact) => artifact.type === Mojom.LINE_CHART_ARTIFACT_TYPE,
          )
          .map((artifact, i) => (
            <Chart
              key={i}
              artifact={artifact}
            />
          ))}
      </div>
    </AssistantResponseContextProvider>
  )
}
