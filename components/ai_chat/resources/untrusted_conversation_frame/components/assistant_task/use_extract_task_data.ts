// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as Mojom from '../../../common/mojom'

export default function useExtractTaskData(
  assistantEntryGroup: Mojom.ConversationTurn[],
) {
  return React.useMemo(() => {
    // Individual tasks, split by CompletionEvent
    const taskItems: Mojom.ConversationEntryEvent[][] = []
    // All completion events are allowed the links provided by the whole response
    // group.
    const allowedLinks = new Set<string>()

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
      }
    }

    return {
      taskItems,
      allowedLinks: Array.from(allowedLinks),
    }
  }, [assistantEntryGroup])
}

export type TaskData = ReturnType<typeof useExtractTaskData>
