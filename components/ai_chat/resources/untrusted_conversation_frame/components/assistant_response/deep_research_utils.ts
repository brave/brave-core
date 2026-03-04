// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../../../common/mojom'

/** Aggregated deep research events extracted from a conversation entry. */
export interface ExtractedDeepResearchEvents {
  queriesEvent?: Mojom.DeepResearchQueriesEvent
  thinkingEvents: Mojom.DeepResearchThinkingEvent[]
  progressEvent?: Mojom.DeepResearchProgressEvent
  completeEvent?: Mojom.DeepResearchCompleteEvent
  errorEvent?: Mojom.DeepResearchErrorEvent
  searchStatusEvent?: Mojom.DeepResearchSearchStatusEvent
  analysisStatusEvent?: Mojom.DeepResearchAnalysisStatusEvent
  iterationCompleteEvent?: Mojom.DeepResearchIterationCompleteEvent
  analyzingEvent?: Mojom.DeepResearchAnalyzingEvent
  fetchStatusEvent?: Mojom.DeepResearchFetchStatusEvent
  hasDeepResearchEvents: boolean
  /** Whether the synthesis answer (completionEvent after the tool call) has arrived. */
  hasSynthesisCompletion: boolean
}

/** Extracts and deduplicates deep research events from conversation events.
 *  When multiple deep research tasks exist, only events after the last
 *  deep research ToolUseEvent are considered. */
export function extractDeepResearchEvents(
  events: Mojom.ConversationEntryEvent[] | undefined,
): ExtractedDeepResearchEvents {
  // Find the index of the last deep research ToolUseEvent so we only
  // consider events from the most recent deep research task.
  let lastToolUseIndex = -1
  events?.forEach((event, i) => {
    if (event.toolUseEvent?.toolName === Mojom.DEEP_RESEARCH_TOOL_NAME) {
      lastToolUseIndex = i
    }
  })
  const hasDeepResearchToolUse = lastToolUseIndex >= 0

  const relevantEvents =
    lastToolUseIndex >= 0 ? events!.slice(lastToolUseIndex) : events

  const drEvents =
    relevantEvents
      ?.filter((event) => event.deepResearchEvent)
      .map((event) => event.deepResearchEvent!) ?? []

  return {
    queriesEvent: drEvents.findLast((e) => e.queriesEvent)?.queriesEvent,
    thinkingEvents: drEvents
      .filter((e) => e.thinkingEvent)
      .map((e) => e.thinkingEvent!),
    progressEvent: drEvents.findLast((e) => e.progressEvent)?.progressEvent,
    completeEvent: drEvents.find((e) => e.completeEvent)?.completeEvent,
    errorEvent: drEvents.find((e) => e.errorEvent)?.errorEvent,
    searchStatusEvent: drEvents.findLast((e) => e.searchStatusEvent)
      ?.searchStatusEvent,
    analysisStatusEvent: drEvents.findLast((e) => e.analysisStatusEvent)
      ?.analysisStatusEvent,
    iterationCompleteEvent: drEvents.findLast((e) => e.iterationCompleteEvent)
      ?.iterationCompleteEvent,
    analyzingEvent: drEvents.findLast((e) => e.analyzingEvent)?.analyzingEvent,
    fetchStatusEvent: drEvents.findLast((e) => e.fetchStatusEvent)
      ?.fetchStatusEvent,
    hasDeepResearchEvents: !!hasDeepResearchToolUse || drEvents.length > 0,
    hasSynthesisCompletion:
      relevantEvents?.some((e) => e.completionEvent) ?? false,
  }
}
