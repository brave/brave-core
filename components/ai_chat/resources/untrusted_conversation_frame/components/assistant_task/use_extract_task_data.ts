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
      }
    }

    return {
      taskItems,
    }
  }, [assistantEntryGroup])
}

export type TaskData = ReturnType<typeof useExtractTaskData>
