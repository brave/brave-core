// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Tabs from '@brave/leo/react/tabs'
import TabItem from '@brave/leo/react/tabItem'
import { getLocale } from '$web-common/locale'
import API from '../../untrusted_conversation_frame_api'
import * as Mojom from '../../../common/mojom'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import AssistantResponse from '.'
import ToolEvent from './tool_event'
import styles from './assistant_task.module.scss'
import classnames from '$web-common/classnames'
import ProgressRing from '@brave/leo/react/progressRing'
import Icon from '@brave/leo/react/icon'

interface Props {
  // Entries that make up the task loop
  assistantEntries: Mojom.ConversationTurn[]
  isActiveTask: boolean
  isGenerating: boolean
}

// Should be Navigate and TODO
const importantToolNames = [Mojom.NAVIGATE_TOOL_NAME]

function getImportantToolUseEvents(
  assistantEntries: Mojom.ConversationTurn[],
): Mojom.ToolUseEvent[] {
  const importantToolUseEvents: Partial<
    Record<(typeof importantToolNames)[number], Mojom.ToolUseEvent>
  > = {}

  for (const event of assistantEntries
    .toReversed()
    .flatMap((entry) => entry.events?.toReversed() ?? [])) {
    if (
      !!event
      && !!event.toolUseEvent
      && importantToolNames.includes(event.toolUseEvent.toolName ?? '')
    ) {
      if (importantToolUseEvents[event.toolUseEvent.toolName]) {
        continue
      }
      importantToolUseEvents[event.toolUseEvent.toolName] = event.toolUseEvent!
      // Don't keep iterating if we've found all the important tool use events
      if (
        Object.keys(importantToolUseEvents).length === importantToolNames.length
      ) {
        break
      }
    }
  }
  return Object.values(importantToolUseEvents).filter((event) => !!event)
}

export default function AssistantTask(props: Props) {
  const [showSteps, setShowSteps] = React.useState(false)
  const [taskThumbnail, setTaskThumbnail] = React.useState<string>()
  const conversationContext = useUntrustedConversationContext()

  return (
    <div className={styles.task}>
      <Tabs
        onChange={() => setShowSteps(!showSteps)}
        value={showSteps ? 'steps' : 'progress'}
        size='medium'
      >
        <TabItem value='progress'>
          {getLocale(S.AI_CHAT_TASK_PROGRESS_LABEL)}
        </TabItem>
        <TabItem value='steps'>{getLocale(S.AI_CHAT_TASK_STEPS_LABEL)}</TabItem>
      </Tabs>
      <div className={styles.taskContent}>
        <div className={styles.taskData}>
          {showSteps && <Steps {...props} />}

          {!showSteps && <Progress {...props} />}
        </div>
        {taskThumbnail && (
          <div className={styles.taskImage}>
            <img src={taskThumbnail} />
          </div>
        )}
      </div>
    </div>
  )
}

function Progress(props: Props) {
  // The Progress tab should show:
  // - Any last active complete "important" tool (TODO, navigate)
  // - the most recent completion event
  // - any tool use events from the most recent entry in the group (the active
  // events)
  const conversationContext = useUntrustedConversationContext()

  const lastCompletionEvent = props.assistantEntries
    .flatMap(
      (entry) =>
        entry.events?.filter((event) => !!event && event.completionEvent) ?? [],
    )
    .at(-1)

  const lastEntryToolUseEvents =
    props.assistantEntries
      .at(-1)
      ?.events?.filter((event) => !!event && !!event.toolUseEvent) ?? []

  const importantToolUseEvents = getImportantToolUseEvents(
    props.assistantEntries,
  )

  return (
    <div>
      {importantToolUseEvents.map((event, index) => (
        <ToolEvent
          key={index}
          toolUseEvent={event}
          isEntryActive={props.isActiveTask}
        />
      ))}
      {lastCompletionEvent && (
        <AssistantResponse
          events={[lastCompletionEvent]}
          isEntryInteractivityAllowed={false}
          isEntryInProgress={props.isGenerating}
          allowedLinks={[]}
          isLeoModel={conversationContext.isLeoModel}
        />
      )}
      {lastEntryToolUseEvents.map((event, index) => (
        <ToolEvent
          key={index}
          toolUseEvent={event.toolUseEvent!}
          isEntryActive={props.isActiveTask}
        />
      ))}
    </div>
  )
}

function Steps(props: Props) {
  // Render every event in the task, split by completion event
  // so that the LLM tells a story of the task by it's own progress
  // description.
  const conversationContext = useUntrustedConversationContext()
  let taskItems: Mojom.ConversationEntryEvent[][] = [[]]
  const allowedLinks: string[] = []
  for (const event of props.assistantEntries.flatMap(
    (entry) => entry.events ?? [],
  )) {
    if (!!event && !!event.completionEvent) {
      taskItems.push([event])
    } else if (event) {
      taskItems[taskItems.length - 1].push(event)
      if (event.sourcesEvent) {
        allowedLinks.push(
          ...event.sourcesEvent.sources.map((source) => source.url.url),
        )
      }
    }
  }

  taskItems = taskItems.filter((taskItem) => taskItem.length > 0)

  return taskItems.map((taskItem, index) => {
    const isActive = index === taskItems.length - 1
    return (
      <div
        key={index}
        className={classnames(styles.taskStep, !isActive && styles.isComplete)}
      >
        <div className={styles.taskIcon}>
          {isActive ? <ProgressRing /> : <Icon name='check-circle-outline' />}
        </div>
        <AssistantResponse
          events={taskItem}
          isEntryInteractivityAllowed={isActive}
          isEntryInProgress={props.isGenerating && isActive}
          allowedLinks={allowedLinks}
          isLeoModel={conversationContext.isLeoModel}
        />
      </div>
    )
  })
}
