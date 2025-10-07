// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as Mojom from '../../../common/mojom'

// Important tools are ones worth always showing the latest version of
// to the user because they convey a longer-term status of the task compared
// to other tools. For example a navigation in a web page vs a click, or an
// up to date TODO list of items in the current task.
// TODO(https://github.com/brave/brave-browser/issues/48535):
// Should be Navigate and TODO tools.
// Note: Navigate could be removed if we display the current Url of a related
// content tab in the Task UI instead (since history navigation affects the
// "current" URL and not just the most recent navigation tool. This is all
// experimental.
const importantToolNames: string[] = [Mojom.NAVIGATE_TOOL_NAME] as const

export default function useExtractTaskData(
  assistantEntryGroup: Mojom.ConversationTurn[],
) {
  return React.useMemo(() => {
    // Individual tasks, split by CompletionEvent
    const taskItems: Mojom.ConversationEntryEvent[][] = []
    // All completion events are allowed the links provided by the whole response
    // group.
    const allowedLinks = new Set<string>()
    // Most recent of each type of important tool use event
    const importantToolUseEvents: Partial<Record<string, Mojom.ToolUseEvent>> =
      {}

    for (const event of assistantEntryGroup.flatMap(
      (entry) => entry.events ?? [],
    )) {
      if (event.completionEvent) {
        // Start a new task item for every completion event
        taskItems.push([event])
      } else {
        if (!taskItems.at(-1)) {
          taskItems.push([])
        }
        // Add any other event types to the last task item
        taskItems.at(-1)?.push(event)
        // Additionally collate the allowed links when a sources event is found
        if (event.sourcesEvent) {
          for (const source of event.sourcesEvent.sources) {
            allowedLinks.add(source.url.url)
          }
        }
        // Overwrite collection of important tool use events with the latest one of
        // each type.
        if (
          event.toolUseEvent
          && importantToolNames.includes(event.toolUseEvent.toolName)
          && event.toolUseEvent.securityMetadataAllowed !== false
        ) {
          importantToolUseEvents[event.toolUseEvent.toolName] =
            event.toolUseEvent
        }
      }
    }

    return {
      taskItems,
      allowedLinks: Array.from(allowedLinks),
      importantToolUseEvents: Object.values(importantToolUseEvents).filter(
        (event) => !!event,
      ), // satisfy that we won't have any undefined value in the array
    }
  }, [assistantEntryGroup])
}

export type TaskData = ReturnType<typeof useExtractTaskData>
