// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../../../common/mojom'

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
}

export function extractDeepResearchEvents(
  events: Mojom.ConversationEntryEvent[] | undefined,
): ExtractedDeepResearchEvents {
  const drEvents = events?.filter(
    (event) => event.deepResearchEvent,
  ).map((event) => event.deepResearchEvent!) ?? []

  const hasDeepResearchToolUse = events?.some(
    (event) => event.toolUseEvent?.toolName === Mojom.DEEP_RESEARCH_TOOL_NAME,
  )

  return {
    queriesEvent: drEvents.findLast(
      (e) => e.queriesEvent,
    )?.queriesEvent,
    thinkingEvents: drEvents
      .filter((e) => e.thinkingEvent)
      .map((e) => e.thinkingEvent!),
    progressEvent: drEvents.findLast(
      (e) => e.progressEvent,
    )?.progressEvent,
    completeEvent: drEvents.find(
      (e) => e.completeEvent,
    )?.completeEvent,
    errorEvent: drEvents.find(
      (e) => e.errorEvent,
    )?.errorEvent,
    searchStatusEvent: drEvents.findLast(
      (e) => e.searchStatusEvent,
    )?.searchStatusEvent,
    analysisStatusEvent: drEvents.findLast(
      (e) => e.analysisStatusEvent,
    )?.analysisStatusEvent,
    iterationCompleteEvent: drEvents.findLast(
      (e) => e.iterationCompleteEvent,
    )?.iterationCompleteEvent,
    analyzingEvent: drEvents.findLast(
      (e) => e.analyzingEvent,
    )?.analyzingEvent,
    fetchStatusEvent: drEvents.findLast(
      (e) => e.fetchStatusEvent,
    )?.fetchStatusEvent,
    hasDeepResearchEvents: !!hasDeepResearchToolUse || drEvents.length > 0,
  }
}
