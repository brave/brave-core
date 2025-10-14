// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as Mojom from '../../../common/mojom'

// TODO(https://github.com/brave/brave-browser/issues/48535):
// Should be Navigate and TODO tools.
const importantToolNames = [Mojom.NAVIGATE_TOOL_NAME]

export default function useExtractTaskData(
  assistantEntryGroup: Mojom.ConversationTurn[],
) {
  return React.useMemo(() => {
    // Individual tasks, split by CompletionEvent
    let taskItems: Mojom.ConversationEntryEvent[][] = [[]]
    // All completion events are allowed the links provided by the whole response
    // group.
    const allowedLinks: string[] = []
    // Most recent of each type of important tool use event
    const importantToolUseEvents: Partial<
      Record<(typeof importantToolNames)[number], Mojom.ToolUseEvent>
    > = {}

    for (const event of assistantEntryGroup.flatMap(
      (entry) => entry.events ?? [],
    )) {
      if (!event) {
        continue
      }
      if (event.completionEvent) {
        // Start a new task item for every completion event
        taskItems.push([event])
      } else {
        // Add any other event types to the last task item
        taskItems[taskItems.length - 1].push(event)
        // Additionally collate the allowed links when a sources event is found
        if (event.sourcesEvent) {
          allowedLinks.push(
            ...event.sourcesEvent.sources.map((source) => source.url.url),
          )
        }
        // Overwrite collection of important tool use events with the latest one of
        // each type.
        if (
          event.toolUseEvent
          && importantToolNames.includes(event.toolUseEvent.toolName)
        ) {
          importantToolUseEvents[event.toolUseEvent.toolName] =
            event.toolUseEvent
        }
      }
    }

    // Filter out empty task items since we start with an empty one and we might
    // have encountered a completion event first, which starts a new task item.
    taskItems = taskItems.filter((taskItem) => taskItem.length > 0)

    return {
      taskItems,
      allowedLinks,
      importantToolUseEvents: Object.values(importantToolUseEvents).filter(
        (event) => !!event,
      ), // satisfy that we won't have any undefined value in the array
    }
  }, [assistantEntryGroup])
}

export type TaskData = ReturnType<typeof useExtractTaskData>
